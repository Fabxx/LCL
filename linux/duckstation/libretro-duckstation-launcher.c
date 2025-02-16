#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libretro.h"
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define ELF_MAGIC "\x7F""ELF"

static uint32_t *frame_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
   frame_buf = calloc(320 * 240, sizeof(uint32_t));
}

void retro_deinit(void)
{
   free(frame_buf);
   frame_buf = NULL;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "duckstation Launcher";
   info->library_version  = "0.1a";
   info->need_fullpath    = true;
   info->valid_extensions = "cue|img|ecm|chd";
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   float aspect = 4.0f / 3.0f;
   float sampling_rate = 30000.0f;

   info->timing = (struct retro_system_timing) {
      .fps = 60.0,
      .sample_rate = sampling_rate,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = 320,
      .base_height  = 240,
      .max_width    = 320,
      .max_height   = 240,
      .aspect_ratio = aspect,
   };
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   bool no_content = true;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   // Nothing needs to happen when the game is reset.
}

/**
 * libretro callback; Called every game tick.
 *
 * Once the core has run, we will attempt to exit, since xemu is done.
 */
void retro_run(void)
{
   // Clear the display.
   unsigned stride = 320;
   video_cb(frame_buf, 320, 240, stride << 2);

   // Shutdown the environment now that xemu has loaded and quit.
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

static int is_elf_executable(const char *filename) {
    
    unsigned char magic[4];
    int fd = open(filename, O_RDONLY);

    if (fd < 0) {
        return 0;
    }

    ssize_t read_bytes = read(fd, magic, 4);
    close(fd);

    return (read_bytes == 4 && memcmp(magic, ELF_MAGIC, 4) == 0);
}

/**
 * Setup emulator directories and try to find executable.
 * If executable was not found then download it.
 * If executable was found check for updates.
 */
static char *setup(char **Paths, size_t numPaths, char *executable)
{
   glob_t buf;
   struct stat path_stat; 

   // Do NOT try to create search path for glob (dirs[6])
   for (size_t i = 0; i < numPaths-1; i++) {
      if (stat(Paths[i], &path_stat) != 0) {
         mkdir(Paths[i], 0755);
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: %s folder created\n", Paths[i]);
      } else {
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: %s folder already exist\n", Paths[i]);
      }
   }

   if (glob(Paths[6], 0, NULL, &buf) == 0) {
         for (size_t i = 0; i < buf.gl_pathc; i++) {
            if (stat(buf.gl_pathv[i], &path_stat) == 0 && !S_ISDIR(path_stat.st_mode)) {
                  if (is_elf_executable(buf.gl_pathv[i])) {
                     //Match size of 513 from retro_load_game() function.
                     snprintf(executable, sizeof(executable)+505, "%s", buf.gl_pathv[i]);
                     log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Found emulator: %s\n", executable);
                     return executable;
               }
            }
         }
         globfree(&buf);
      } 
      
      log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Downloading emulator.\n");
      executable = "";
      return executable;
}

static bool downloader(char **Paths, char **downloaderDirs, char **githubUrls, char *executable, size_t numPaths)
{
   char url[260] = {0}, currentVersion[32] = {0}, newVersion[32] = {0};
   char bashCommand[1024] = {0}, downloadCmd[1024] = {0}; 

   if (strlen(executable) == 0) {

      char bashCommand[1024] = {0};

      snprintf(bashCommand, sizeof(bashCommand),
   "bash -c 'json_data=$(curl -s -H \"Accept: application/json\" \"%s\"); "
         "tag=$(echo \"$json_data\" | jq -r \".tag_name\"); "
         "name=$(echo \"$json_data\" | jq -r \".assets[9].name\"); "
         "id=$(echo \"$json_data\" | jq -r \".assets[9].id\"); "
         "if [ -z \"$tag\" ] || [ -z \"$name\" ] || [ \"$tag\" = \"null\" ] || [ \"$id\" = \"null\" ]  || [ \"$name\" = \"null\" ]; then exit 1; fi; "
         "url=\"%s$tag/$name\"; "
         "echo \"$url\" > \"%s\";"
         "echo \"$id\"  > \"%s\"'",
         githubUrls[0], githubUrls[1], downloaderDirs[0], downloaderDirs[1]);

      if (system(bashCommand) != 0) {
            log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to fetch download URL, aborting.\n");
            return false;
         } else { // If it's first download, extract only URL for download. Current version is already saved by bash.
            FILE *urlFile = fopen(downloaderDirs[0], "r");
            
            if (!urlFile) {
               log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to read version file, aborting.\n");
               return false;
            } else {
               fgets(url, sizeof(url), urlFile);
               fclose(urlFile);
               url[strcspn(url, "\n")] = 0;

            }
         }

      snprintf(downloadCmd, sizeof(downloadCmd), 
      "wget -O %s/duckstation.AppImage %s && chmod +x %s/duckstation.AppImage", Paths[0], url, Paths[0]);
      
      if (system(downloadCmd) != 0) {
            log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to download emulator, aborting.\n");
            return false;
         }
   } else { // If it's not the first download, fetch newVersion ID.
         snprintf(bashCommand, sizeof(bashCommand),
       "bash -c 'json_data=$(curl -s -H \"Accept: application/json\" \"%s\"); "
               "tag=$(echo \"$json_data\" | jq -r \".tag_name\"); "
               "name=$(echo \"$json_data\" | jq -r \".assets[9].name\"); "
               "id=$(echo \"$json_data\" | jq -r \".assets[9].id\"); "
               "if [ -z \"$tag\" ] || [ -z \"$name\" ] || [ \"$tag\" = \"null\" ] || [ \"$id\" = \"null\" ]  || [ \"$name\" = \"null\" ]; then exit 1; fi; "
               "url=\"%s$tag/$name\"; "
               "echo \"$url\" > \"%s\";"
               "echo \"$id\"  > \"%s\"'",
               githubUrls[0], githubUrls[1], downloaderDirs[0], downloaderDirs[2]);
         
         if (system(bashCommand) != 0) {
            log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to fetch update, aborting.\n");
            return false;
         } else { // Extract URL, currentID and newID for comparison
            FILE *urlFile = fopen(downloaderDirs[0], "r");
            FILE *currentVersionFile = fopen(downloaderDirs[1], "r");
            FILE *newVersionFile = fopen(downloaderDirs[2], "r");
            
            if (!urlFile && !currentVersionFile && ! newVersionFile) {
               log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Metadata files not found. Aborting.\n");
               return false;
            } else {
               fgets(url, sizeof(url), urlFile);
               fgets(currentVersion, sizeof(currentVersion), currentVersionFile);
               fgets(newVersion, sizeof(newVersion), newVersionFile);
               fclose(urlFile);
               fclose(currentVersionFile);
               fclose(newVersionFile);
               
               url[strcspn(url, "\n")] = 0;

               if (strcmp(currentVersion, newVersion) != 0) {
                  log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Update found. Downloading Update\n");
                  snprintf(downloadCmd, sizeof(downloadCmd), 
                  "wget -O %s/duckstation.AppImage %s && chmod +x %s/duckstation.AppImage", Paths[0], url, Paths[0]);
                  
                  if (system(downloadCmd) != 0) {
                     log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to download update, aborting.\n");
                     return false;
                  } else {
                     // Overwrite Current version.txt file with new ID if download was successfull.
                     snprintf(bashCommand, sizeof(bashCommand),
                   "bash -c 'json_data=$(curl -s -H \"Accept: application/json\" \"%s\"); "
                           "id=$(echo \"$json_data\" | jq -r \".assets[9].id\"); "
                           "if [ \"$id\" = \"null\" ]; then exit 1; fi; "
                           "url=\"%s$tag/$name\"; "
                           "echo \"$id\"  > \"%s\"'",
                           githubUrls[0], githubUrls[1], downloaderDirs[1]);
                  
                     if (system(bashCommand) != 0) {
                        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to update current version file. Aborting.\n");
                        return false;
                     } else {
                        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Success.\n");
                     }
                  }
               } else {
                  log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: No update found.\n");
               }
            }
         }
   }
   return true;
}

/**
 * libretro callback; Called when a game is to be loaded.
*  - attach ROM absolute path from info->path in double quotes for system() function, avoids truncation.
*  - if info->path has no ROM, fallback to bios file placed by the user.
   NOTE: info structure must be checked when is not null
 */
bool retro_load_game(const struct retro_game_info *info)
{

   // Default Emulator Paths
   char *dirs[] = {
         "/.config/retroarch/system/duckstation",
         "/.config/retroarch/system/duckstation/bios",
         "/.config/retroarch/thumbnails/Sony - Playstation",
         "/.config/retroarch/thumbnails/Sony - Playstation/Named_Boxarts",
         "/.config/retroarch/thumbnails/Sony - Playstation/Named_Snaps",
         "/.config/retroarch/thumbnails/Sony - Playstation/Named_Titles",
         "/.config/retroarch/system/duckstation/duckstation*" // search Path for glob.
      };

   // Emulator build versions and URL to download. Content is generated from powershell cmds
   char *downloaderDirs[] = {
      "/.config/retroarch/system/duckstation/0.Url.txt",
      "/.config/retroarch/system/duckstation/1.CurrentVersion.txt",
      "/.config/retroarch/system/duckstation/2.NewVersion.txt",
   };

   char *githubUrls[] = {
      "https://api.github.com/repos/stenzek/duckstation/releases/latest",
      "https://github.com/stenzek/duckstation/releases/download/"
   };

   char executable[513] = {0};
   const char *home = getenv("HOME");
   size_t numPaths = sizeof(dirs)/sizeof(char*);
   size_t dirPaths = sizeof(downloaderDirs)/sizeof(char*);

   // Concat retreived HOME path for dirs.

   for (size_t i = 0; i < numPaths; i++) {
      char *tmp = dirs[i];
      asprintf(&dirs[i], "%s%s", home, tmp );
   }

   for (size_t i = 0; i < dirPaths; i++) {
      char *tmp = downloaderDirs[i];
      asprintf(&downloaderDirs[i], "%s%s", home, tmp );
   }

   setup(dirs, numPaths, executable);
   downloader(dirs, downloaderDirs, githubUrls, executable, numPaths);

   // if executable exists, only then try to launch it.
   if (strlen(executable) > 0) {
      if (info == NULL || info->path == NULL) {
         char args[512] = {0};
         snprintf(args, sizeof(args), " -fullscreen -bios");
         strncat(executable, args, sizeof(executable)-1);
      } else {
         char args[512] = {0};
         snprintf(args, sizeof(args), " -fullscreen \"%s\"", info->path);
         strncat(executable, args, sizeof(executable)-1);
      }

      if (system(executable) == 0) {
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Finished running duckstation.\n");
         return true;
      } else {
         log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed running duckstation.\n");
      }
   }
   return false;
}

void retro_unload_game(void)
{
   // Nothing needs to happen when the game unloads.
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   return retro_load_game(info);
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return true;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{
   // Nothing.
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}
