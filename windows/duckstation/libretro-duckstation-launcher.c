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

/**
 * libretro callback; Called when a game is to be loaded.
 *  
 *  - Windows:
 *       - create dir for emulator files and bios
		 - setup folders
 *       - search for .exe binary with name pattern.
 *		
 * - Final Steps:
 *       - attach ROM absolute path from info->path in double quotes for system() function, avoids truncation.
 *       - if info->path has no ROM, fallback to bios file placed by the user.
         NOTE: info structure must be checked when is not null
 */
bool retro_load_game(const struct retro_game_info *info)
{
      WIN32_FIND_DATA findFileData;
      HANDLE hFind;
      char emuPath[MAX_PATH] = "C:\\RetroArch-Win64\\system\\duckstation";
      char biosPath[MAX_PATH] = "C:\\RetroArch-Win64\\system\\duckstation\\bios";
      char thumbnailsPath[MAX_PATH] = "C:\\RetroArch-Win64\\thumbnails\\Sony - Playstation";
      char executable[MAX_PATH] = {0};
      char searchPath[MAX_PATH] = {0};
      const char *thumbDirs[] = {"\\Named_Boxarts", "\\Named_Snaps", "\\Named_Titles"};
      char currentVersionLocation[512] = {0};
      char newVersionLocation[512] = {0};
      char urlLocation[512] = {0};

      // Initialize paths for updater and downloader.
      snprintf(currentVersionLocation, sizeof(currentVersionLocation), "%s\\currentId.txt", emuPath);
      snprintf(newVersionLocation, sizeof(newVersionLocation), "%s\\newId.txt", emuPath);
      snprintf(urlLocation, sizeof(urlLocation), "%s\\url.txt", emuPath);

      // Create emulator folder if it doesn't exist
      if (GetFileAttributes(emuPath) == INVALID_FILE_ATTRIBUTES) {
         _mkdir(emuPath);
         printf("[LAUNCHER-INFO]: Emulator folder created in %s\n", emuPath);
      } else {
         printf("[LAUNCHER-INFO]: Emulator folder already exists\n");
      }

      // Create BIOS folder if it doesn't exist
      if (GetFileAttributes(biosPath) == INVALID_FILE_ATTRIBUTES) {
         _mkdir(biosPath);
         printf("[LAUNCHER-INFO]: BIOS folder created in %s\n", biosPath);
      } else {
         printf("[LAUNCHER-INFO]: BIOS folder already exists\n");
      }

      // Create thumnbnails folder if it doesn't exist
      if (GetFileAttributes(thumbnailsPath) == INVALID_FILE_ATTRIBUTES) {
         _mkdir(thumbnailsPath);
         printf("[LAUNCHER-INFO]: Thumbnails folder created in %s\n", thumbnailsPath);
      } else {
         printf("[LAUNCHER-INFO]: Thumbanils folder already exists\n");
      }

      // Create Thumbnail directories
      for (size_t i = 0; i < sizeof(thumbDirs)/sizeof(thumbDirs[0]); i++) {
         char fullPath[MAX_PATH] = {0};
         snprintf(fullPath, sizeof(fullPath), "%s%s", thumbnailsPath, thumbDirs[i]);
         if (GetFileAttributes(fullPath) == INVALID_FILE_ATTRIBUTES) {
            _mkdir(fullPath);
            printf("[LAUNCHER-INFO]: %s folder created\n", fullPath);
         } else {
            printf("[LAUNCHER-INFO]: %s folder already exists\n", fullPath);
         }
      }

      // Search for binary executable
      snprintf(searchPath, MAX_PATH, "%s\\duckstation*.exe", emuPath);
      hFind = FindFirstFile(searchPath, &findFileData);

      if (hFind != INVALID_HANDLE_VALUE) {
         snprintf(executable, MAX_PATH, "%s\\%s", emuPath, findFileData.cFileName);
         FindClose(hFind);
         printf("[LAUNCHER-INFO]: Found emulator: %s\n", executable);

         /*If executable was found, check for updates by comparing current URL ID with the last saved URL ID. 
           Build the new download URL with the new tag if any.
           
           If the IDs are different use the new fetched URL to download

           This is needed because on duckstation the rolling releases are on the same tag 
           called "latest", so if you try to compare the URLs those will be always equal.
          */ 
         int currentId = 0;
         int newId = 0;
         char urlString[MAX_PATH];
         char psCommand[MAX_PATH * 3] = {0};

         snprintf(psCommand, sizeof(psCommand),
         "powershell -Command \"$response = (Invoke-WebRequest -Uri 'https://api.github.com/repos/stenzek/duckstation/releases/latest' -Headers @{Accept='application/json'}).Content | ConvertFrom-Json; "
               "$tag = $response.tag_name;"
               "$name = $response.assets[5].name;"
               "$url = 'https://github.com/stenzek/duckstation/releases/download/' + $tag + '/' + $name; "
               "$id  = $response.assets[5].id;"
               "Write-Output $url\"  > \"%s\";"
               "Write-Output $id\"   >  \"%s\"", urlLocation, newVersionLocation);

         if (system(psCommand) != 0) {
            printf("[LAUNCHER-ERROR]: Failed to fetch new version, aborting.\n");
            return false;
         }
          
         FILE *currentVersion = fopen(currentVersionLocation, "r");
         FILE *newVersion = fopen(newVersionLocation, "r");
         FILE *url = fopen(urlLocation, "r");
         
         if (currentVersion && newVersion && url) {
            fread(&currentId, sizeof(currentId), 1, currentVersion);
            fread(&newId, sizeof(newId), 1, newVersion);
            fgets(urlString, sizeof(urlString), url);
            fclose(currentVersion);
            fclose(newVersion);
            fclose(url);
         } else {
            printf("[LAUNCHER-ERROR]: Failed to fetch update, aborting.\n");
            return false;
         }

         if (currentId != newId) {
             printf("[LAUNCHER-INFO]: duckstation update Found, new URL ID: %d\n", newId);
             printf("[LAUNCHER-INFO]: current duckstation URL ID: %d\n", currentId);

             char downloadCmd[MAX_PATH * 2] = {0};
             snprintf(downloadCmd, sizeof(downloadCmd),
          "powershell -Command \"Invoke-WebRequest -Uri '%s' -OutFile '%s\\duckstation.zip'\"", urlString, emuPath);
         
         if (system(downloadCmd) != 0) {
            printf("[LAUNCHER-ERROR]: Failed to download emulator, aborting.\n");
            return false;
         } else {
            printf("[LAUNCHER-INFO]: Download successful, extracting emulator.\n");
           
            char extractCmd[MAX_PATH * 2] = {0};
            snprintf(extractCmd, sizeof(extractCmd),
             "powershell -Command \"Expand-Archive -Path '%s\\duckstation.zip' -DestinationPath '%s' -Force; Remove-Item -Path '%s\\duckstation.zip' -Force\"", emuPath, emuPath, emuPath);
            
            if (system(extractCmd) != 0) {
               printf("[LAUNCHER-ERROR]: Failed to extract emulator, aborting.\n");
               return false;
            }
            printf("[LAUNCHER-INFO]: Success, rebooting RetroArch...\n");
            return false;
         }
         } else {
            printf("[LAUNCHER-INFO]: No duckstation updates found.\n");
         }

      } else {
         printf("[LAUNCHER-INFO]: No executable found, downloading emulator.\n");

         /* If no emulator was downloaded before, download directly from last available URL 
            Retreiving the release URL ID to check if the release has changed for the updater.
            Each release generates a different ID for the same URL.
         */

         char url[MAX_PATH];
         char psCommand[MAX_PATH * 3] = {0};
         snprintf(psCommand, sizeof(psCommand),
         "powershell -Command \"$response = (Invoke-WebRequest -Uri 'https://api.github.com/repos/stenzek/duckstation/releases/latest' -Headers @{Accept='application/json'}).Content | ConvertFrom-Json; "
               "$tag = $response.tag_name;"
               "$name = $response.assets[5].name;"
               "$id = $response.assets[5].id;"
               "$url = 'https://github.com/stenzek/duckstation/releases/download/' + $tag + '/' + $name; "
               "Write-Output $url\" > \"%s\";"
               "Write-Output $id\" > \"%s\"", urlLocation, currentVersionLocation);

         if (system(psCommand) != 0) {
            printf("[LAUNCHER-ERROR]: Failed to fetch latest version, aborting.\n");
            return false;
         }

         FILE *file = fopen(urlLocation, "r");
         if (file) {
            fgets(url, sizeof(url), file);
            fclose(file);
         } else {
            printf("[LAUNCHER-ERROR]: Failed to read version file, aborting.\n");
            return false;
         }

         url[strcspn(url, "\r\n")] = 0;

         printf("[LAUNCHER-INFO]: Latest duckstation release URL: %s\n", url);
         
         char downloadCmd[MAX_PATH * 2] = {0};
         snprintf(downloadCmd, sizeof(downloadCmd),
          "powershell -Command \"Invoke-WebRequest -Uri '%s' -OutFile '%s\\duckstation.zip'\"", url, emuPath);
         
         if (system(downloadCmd) != 0) {
            printf("[LAUNCHER-ERROR]: Failed to download emulator, aborting.\n");
            return false;
         } else {
            printf("[LAUNCHER-INFO]: Download successful, extracting emulator.\n");
           
            char extractCmd[MAX_PATH * 2] = {0};
            snprintf(extractCmd, sizeof(extractCmd),
             "powershell -Command \"Expand-Archive -Path '%s\\duckstation.zip' -DestinationPath '%s' -Force; Remove-Item -Path '%s\\duckstation.zip' -Force\"", emuPath, emuPath, emuPath);
            
            if (system(extractCmd) != 0) {
               printf("[LAUNCHER-ERROR]: Failed to extract emulator, aborting.\n");
               return false;
            }
            printf("[LAUNCHER-INFO]: Success, rebooting RetroArch...\n");
            return false;
         }
      }

      if (info == NULL || info->path == NULL) {
            char args[512] = {0};
            snprintf(args, sizeof(args), " -fullscreen -bios");
            strncat(executable, args, sizeof(executable)-1);
      } else {
            char args[512] = {0};
            snprintf(args, sizeof(args), " -fullscreen -bios \"%s\"", info->path);
            strncat(executable, args, sizeof(executable)-1);
      }

   if (system(executable) == 0) {
      printf("[LAUNCHER-INFO]: Finished running duckstation.\n");
      return true;
   }

   printf("[LAUNCHER-INFO]: Failed running duckstation.\n");
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