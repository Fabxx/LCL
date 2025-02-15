#include <minwindef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libretro.h"
#include <windows.h>
#include <direct.h>

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
   info->library_name     = "PPSSPP Launcher";
   info->library_version  = "0.1a";
   info->need_fullpath    = true;
   info->valid_extensions = "elf|iso|cso|prx|pbp|chd";
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


// Setup emulator dirs and try to find executable.
static char *setup(char **Paths, size_t numPaths, char *executable)
{
   WIN32_FIND_DATA findFileData;
   HANDLE hFind;
   char searchPath[MAX_PATH] = {0};

   log_cb(RETRO_LOG_INFO, "SIZE: %llu\n", Paths);

   // Create Default Dirs if they don't exist.
   for (size_t i = 0; i < numPaths; i++) {
      if (GetFileAttributes(Paths[i]) == INVALID_FILE_ATTRIBUTES) {
         _mkdir(Paths[i]);
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: created folder in %s\n", Paths[i]);
      } else {
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: %s folder already exists\n", Paths[i]);
      }
   }

   // Lookup for Emulator Executable inside Emulator folder. hFind resolves wildcard.
   snprintf(searchPath, MAX_PATH, "%s\\PPSSPPWindows.exe", Paths[0]);
   hFind = FindFirstFile(searchPath, &findFileData);

   if (hFind != INVALID_HANDLE_VALUE) {
         snprintf(executable, MAX_PATH+1, "%s\\%s", Paths[0], findFileData.cFileName);
         FindClose(hFind);
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Found emulator: %s\n", executable);
         return executable;
   } else {
      log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Downloading emulator.\n");
      executable = "";
      return executable;
   }
}

/* in case of PPSSPP, we get lastes release LINK from the official website. GitHub artifacts for windows are not available
   a direct approach is to parse all available Links, and select the most recent one which contains
   the lastes build
*/
static bool downloader(char **dirs, char **downloaderDirs, char **githubUrls, char *executable, size_t numPaths)
{

   char url[260] = {0}, currentUrl[260] = {0};
   char psCommand[MAX_PATH * 3] = {0}, downloadCmd[MAX_PATH * 2] = {0}; 

   setup(dirs, numPaths, executable);

   if (strlen(executable) == 0) {
         snprintf(psCommand, sizeof(psCommand),
     "powershell -Command \""
            "$release = Invoke-WebRequest -Uri 'https://www.ppsspp.org/download/' -UseBasicParsing; "
            "$downloadLinks = $release.Links | Where-Object { $_.href -match 'files/.*/ppsspp_win.zip' } | Select-Object -ExpandProperty href; "
            "$latestLink = $downloadLinks | Sort-Object -Descending | Select-Object -First 1; "
            "if ($latestLink -and $latestLink -notmatch '^https?://') { $latestLink = 'https://www.ppsspp.org' + $latestLink }; "
            "[System.IO.File]::WriteAllText('%s', $latestLink, [System.Text.Encoding]::ASCII)", downloaderDirs[0]);
      
      if (system(psCommand) != 0) {
            log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to fetch download URL, aborting.\n");
            return false;
         } else { // If it's first download, extract only URL for download. Current version is already saved by powershell.
               FILE *urlFile = fopen(downloaderDirs[0], "r");

               if (!urlFile) {
                  log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Powershell failed to export ID of download URL. Aborting.\n");
                  return false;
               } else {
                   fgets(url, sizeof(url), urlFile);
                   fclose(urlFile);
               }
         }
         
         snprintf(downloadCmd, sizeof(downloadCmd),
          "powershell -Command \"Invoke-WebRequest -Uri '%s' -OutFile '%s\\PPSSPP.zip'\"", url, dirs[0]);
         
         if (system(downloadCmd) != 0) {
            log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to download emulator, aborting.\n");
            return false;
         } else {
            log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Download successful, extracting emulator.\n");
            return true;
         }
      } else { // If it's not the first download, fetch Link URL.
            snprintf(psCommand, sizeof(psCommand),
             "powershell -Command \""
                     "$release = Invoke-WebRequest -Uri 'https://www.ppsspp.org/download/' -UseBasicParsing; "
                     "$downloadLinks = $release.Links | Where-Object { $_.href -match 'files/.*/ppsspp_win.zip' } | Select-Object -ExpandProperty href; "
                     "$latestLink = $downloadLinks | Sort-Object -Descending | Select-Object -First 1; "
                     "if ($latestLink -and $latestLink -notmatch '^https?://') { $latestLink = 'https://www.ppsspp.org' + $latestLink }; "
                     "[System.IO.File]::WriteAllText('%s', $latestLink, [System.Text.Encoding]::ASCII)", downloaderDirs[1]);
      
      if (system(psCommand) != 0) {
            log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to fetch update, aborting.\n");
            return false;
         } else { // Extract URL, currentID and newID for comparison
               FILE *urlFile = fopen(downloaderDirs[0], "r");
               FILE *currentVersionFile = fopen(downloaderDirs[1], "r");
   
               if (!urlFile && !currentVersionFile) {
                   log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Metadata files not found. Aborting.\n");
                   return false;
               }

               fgets(url, sizeof(url), urlFile);
               fgets(currentUrl, sizeof(currentUrl), currentVersionFile);

               fclose(urlFile);
               fclose(currentVersionFile);
                
               if (strcmp(url, currentUrl) != 0) {
                  log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Update found. Downloading Update\n");
                  snprintf(downloadCmd, sizeof(downloadCmd),
                  "powershell -Command \"Invoke-WebRequest -Uri '%s' -OutFile '%s\\PPSSPP.zip'\"", url, dirs[0]);
            
               if (system(downloadCmd) != 0) {
                  log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to download update, aborting.\n");
                  return false;
               } else {
                  // Overwrite Current url.txt file with new download Link if download was successfull.
                  snprintf(psCommand, sizeof(psCommand),
             "powershell -Command \""
                     "$release = Invoke-WebRequest -Uri 'https://www.ppsspp.org/download/' -UseBasicParsing; "
                     "$downloadLinks = $release.Links | Where-Object { $_.href -match 'files/.*/ppsspp_win.zip' } | Select-Object -ExpandProperty href; "
                     "$latestLink = $downloadLinks | Sort-Object -Descending | Select-Object -First 1; "
                     "if ($latestLink -and $latestLink -notmatch '^https?://') { $latestLink = 'https://www.ppsspp.org' + $latestLink }; "
                     "[System.IO.File]::WriteAllText('%s', $latestLink, [System.Text.Encoding]::ASCII)", downloaderDirs[0]);
                  
                  if (system(psCommand) != 0) {
                     log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to update current version file. Aborting.\n");
                     return false;
                  }
                  log_cb(RETRO_LOG_ERROR, "[LAUNCHER-INFO]: Download successful, extracting update.\n");
                  return true;
               }
         } 
      }
   }
   log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: No update found.\n");
   return false;
}
   

static bool extractor(char **dirs)
{
   char extractCmd[MAX_PATH * 2] = {0};
   snprintf(extractCmd, sizeof(extractCmd),
            "powershell -Command \"Expand-Archive -Path '%s\\PPSSPP.zip' -DestinationPath '%s' -Force; Remove-Item -Path '%s\\PPSSPP.zip' -Force\"", 
            dirs[0], dirs[0], dirs[0]);
            
   if (system(extractCmd) != 0) {
      log_cb(RETRO_LOG_ERROR,"[LAUNCHER-ERROR]: Failed to extract emulator, aborting.\n");
      return false;
   } else {
      log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Success, running emulator.\n");
      return true; // if false will close core.
   }
}


/**
 * libretro callback; Called when a game is to be loaded. 
  - attach ROM absolute path from info->path in double quotes for system() function, avoids truncation.
  - if info->path has no ROM, fallback to bios file placed by the user.
  NOTE: info structure must be checked when is not null
 */

bool retro_load_game(const struct retro_game_info *info)
{

   // Default Emulator Paths
   char *dirs[] = {
         "C:\\RetroArch-Win64\\system\\PPSSPP",
         "C:\\RetroArch-Win64\\system\\PPSSPP\\bios",
         "C:\\RetroArch-Win64\\thumbnails\\Sony - Playstation Portable",
         "C:\\RetroArch-Win64\\thumbnails\\Sony - Playstation Portable\\Named_Boxarts",
         "C:\\RetroArch-Win64\\thumbnails\\Sony - Playstation Portable\\Named_Snaps",
         "C:\\RetroArch-Win64\\thumbnails\\Sony - Playstation Portable\\Named_Titles"
      };

   size_t numPaths = sizeof(dirs)/sizeof(char*);

   // Emulator build versions and URL to download. Content is generated from powershell cmds
   char *downloaderDirs[] = {
      "C:\\RetroArch-Win64\\system\\PPSSPP\\0.Url.txt",
      "C:\\RetroArch-Win64\\system\\PPSSPP\\1.CurrentVersion.txt",
      "C:\\RetroArch-Win64\\system\\PPSSPP\\2.NewVersion.txt",
      
   };

   char *githubUrls[] = {
      "https://www.ppsspp.org/download/",
      "https://www.ppsspp.org"
   };

   char executable[513] = {0};

   setup(dirs, numPaths, executable);
   
   if (downloader(dirs, downloaderDirs, githubUrls, executable, numPaths)) {
      extractor(dirs);
   }
   
   if (info == NULL || info->path == NULL) {
         char args[512] = {0};
         snprintf(args, sizeof(args), " --fullscreen");
         strncat(executable, args, sizeof(executable)-1);
   } else {
         char args[512] = {0};
         snprintf(args, sizeof(args), " -fullscreen \"%s\"", info->path);
         strncat(executable, args, sizeof(executable)-1);
   }

   if (system(executable) == 0) {
      log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Finished running PPSSPP.\n");
      return true;
   }

   log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed running PPSSPP.\n");
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