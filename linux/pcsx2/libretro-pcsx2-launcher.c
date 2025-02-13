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
   info->library_name     = "pcsx2 Launcher";
   info->library_version  = "0.1a";
   info->need_fullpath    = true;
   info->valid_extensions = "iso|chd|elf|ciso|cso|bin|cue|mdf|nrg|dump|gz|img|m3u";
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
 * libretro callback; Called when a game is to be loaded.
 *  - Linux
 *        - resolve HOME path 
 *        - create dir for emulator files and bios
 *        - apply regex search with glob, filter by file and ELF executable
 *
 * - Final Steps:
 *       - attach ROM absolute path from info->path in double quotes for system() function, avoids truncation.
 *       - if info->path has no ROM, fallback to bios file placed by the user.
         NOTE: info structure must be checked when is not null
 */
bool retro_load_game(const struct retro_game_info *info)
{

      glob_t buf;
      struct stat path_stat;
      char executable[512] = {0};
      char path[512] = {0};
      const char *home = getenv("HOME");

      if (!home) {
         return false;
      }

      /**
       * Create thumbnail directories if they don't exist:

         - System name directory
         - Named_Boxart directory
         - Named_Snaps directory
         - Named_Titles
       * 
       */

      snprintf(path, sizeof(path), "%s/.config/retroarch/thumbnails/Sony - Playstation 2", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: thumbnail folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: thumbnail folder already exist\n");
      }

      snprintf(path, sizeof(path), "%s/.config/retroarch/thumbnails/Sony - Playstation 2/Named_Boxarts", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: Boxart folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: Boxart folder already exist\n");
      }

      snprintf(path, sizeof(path), "%s/.config/retroarch/thumbnails/Sony - Playstation 2/Named_Snaps", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: Snaps folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: Snaps folder already exist\n");
      }

      snprintf(path, sizeof(path), "%s/.config/retroarch/thumbnails/Sony - Playstation 2/Named_Titles", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: Titles folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: Titles folder already exist\n");
      }
      
      // Create emulator folder if it doesn't exist
      snprintf(path, sizeof(path), "%s/.config/retroarch/system/pcsx2", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: emulator folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: emulator folder already exist\n");
      }

      // search for binary executable.
      char emuList[512] = {0};

      snprintf(emuList, sizeof(emuList), "%s/.config/retroarch/system/pcsx2/pcsx2.AppImage", home);

      if (glob(emuList, 0, NULL, &buf) == 0) {
         for (size_t i = 0; i < buf.gl_pathc; i++) {
            if (stat(buf.gl_pathv[i], &path_stat) == 0 && !S_ISDIR(path_stat.st_mode)) {
                  if (is_elf_executable(buf.gl_pathv[i])) {
                     snprintf(executable, sizeof(executable), "%s", buf.gl_pathv[i]);
                     printf("[LAUNCHER-INFO]: Found emulator: %s\n", executable);
                     break;
               }
            }
         }
         globfree(&buf);
      }

      // if no executable was found, download the emulator and make it executable, then close core.
      if (strlen(executable) == 0) {
         printf("[LAUNCHER-INFO]: No executable found, downloading emulator.\n");

         // Retreive lastes version from URL

         char url[256] = {0};
         char bashCommand[1024] = {0};

         snprintf(bashCommand, sizeof(bashCommand),
    "bash -c 'json_data=$(curl -s -H \"Accept: application/json\" \"https://api.github.com/repos/PCSX2/pcsx2/releases/latest\"); "
            "tag=$(echo \"$json_data\" | jq -r \".tag_name\"); "
            "name=$(echo \"$json_data\" | jq -r \".assets[0].name\"); "
            "if [ -z \"$tag\" ] || [ -z \"$name\" ] || [ \"$tag\" = \"null\" ] || [ \"$name\" = \"null\" ]; then exit 1; fi; "
            "url=\"https://github.com/PCSX2/pcsx2/releases/download/$tag/$name\"; "
            "echo \"$url\" > version.txt'");



         if (system(bashCommand) != 0) {
            printf("[LAUNCHER-ERROR]: Failed to fetch latest version, aborting.\n");
            return 1;
         }

         FILE *file = fopen("version.txt", "r");
         if (file) {
            fgets(url, sizeof(url), file);
            fclose(file);
            remove("version.txt");
         } else {
            printf("[LAUNCHER-ERROR]: Failed to read version file, aborting.\n");
            return false;
         }

         url[strcspn(url, "\r\n")] = 0;  // Rimuove newline

         printf("[LAUNCHER-INFO]: Latest pcsx2 release URL: %s\n", url);

         char download[256] = {0};
         snprintf(download, sizeof(download), "wget -O %s/pcsx2.AppImage %s", path, url);

         if (system(download) != 0) {
            printf("[LAUNCHER-ERROR]: Failed to download emulator, aborting.\n");
            return false;
         } else {
               char chmod[512] = {0};
               snprintf(chmod, sizeof(chmod), "chmod +x %s/pcsx2.AppImage", path);

               printf("[LAUNCHER-INFO]: Setting execution permission on executable.\n");

               if (system(chmod) != 0) {
                  printf("[LAUNCHER-ERROR]: Failed to set executable permissions, aborting.\n");
                  return false;
               } else {
                  printf("[LAUNCHER-INFO]: Success, rebooting retroarch...\n");
                  return true;
               }
            }
         }

      // Create bios folder if it doesn't exist
      snprintf(path, sizeof(path), "%s/.config/retroarch/system/pcsx2/bios", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: BIOS folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: BIOS folder already exist\n");
      }

      // Fallback to BIOS if "run core" is selected

      if (info == NULL || info->path == NULL) {
            char args[128] = {0};
            snprintf(args, sizeof(args), " -fullscreen -bios");
            strncat(executable, args, sizeof(executable)-1);
      } else {
         char args[256] = {0};
         snprintf(args, sizeof(args), " -fullscreen \"%s\"", info->path);
         strncat(executable, args, sizeof(executable)-1);
      }

   if (system(executable) == 0) {
      printf("[LAUNCHER-INFO]: Finished running pcsx2.\n");
      return true;
   }

   printf("[LAUNCHER-INFO]: Failed running pcsx2.\n");
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
