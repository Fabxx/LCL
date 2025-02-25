#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libretro.h"

#if defined __linux || __APPLE__
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#elif defined __WIN32__
#include <windows.h>
#include <direct.h>
#endif

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
   info->library_name     = "Xenia Launcher";
   info->library_version  = "0.1a";
   info->need_fullpath    = true;
   info->valid_extensions = "iso|xex|zar";
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
 * Once the core has run, we will attempt to exit, since Xenia Canary Netplay is done.
 */
void retro_run(void)
{
   // Clear the display.
   unsigned stride = 320;
   video_cb(frame_buf, 320, 240, stride << 2);

   // Shutdown the environment now that Xenia Canary Netplay has loaded and quit.
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

/**
 * Setup emulator directories and try to find executable.
 * If executable was not found then download it.
 * If executable was found check for updates.
 */

static bool setup(char **Paths, size_t numPaths, char *executable)
{
   #if defined __linux__ || __APPLE__
   
   glob_t buf;
   struct stat path_stat; 
   
   //Don't create dirs[6]
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
                     //Match size of 513 from retro_load_game() function.
                     snprintf(executable, sizeof(executable)+505, "wine %s", buf.gl_pathv[i]);
                     log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Found emulator: %s\n", executable);
                     return true;
            }
         }
         globfree(&buf);
      }

   #elif defined __WIN32__

   WIN32_FIND_DATA findFileData;
   HANDLE hFind;
   char searchPath[MAX_PATH] = {0};

   for (size_t i = 0; i < numPaths; i++) {
      if (GetFileAttributes(Paths[i]) == INVALID_FILE_ATTRIBUTES) {
         _mkdir(Paths[i]);
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: created folder in %s\n", Paths[i]);
      } else {
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: %s folder already exists\n", Paths[i]);
      }
   }

   // Lookup for Emulator Executable inside Emulator folder. hFind resolves wildcard.
   snprintf(searchPath, MAX_PATH, "%s", Paths[6]);
   hFind = FindFirstFile(searchPath, &findFileData);

   if (hFind != INVALID_HANDLE_VALUE) {
         snprintf(executable, MAX_PATH*2, "%s", Paths[6]);
         FindClose(hFind);
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Found emulator: %s\n", executable);
         return true;
   }
   #endif

    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Downloading emulator.\n");
    return false;
}

static bool downloader(char **Paths, char **downloaderDirs, char **githubUrls)
{
   char url[260] = {0}, command[1024] = {0};
   
   #ifdef __linux__

   char *assetId = "0";

   #elif defined __APPLE__

   char *assetId = "0";

   #elif defined __WIN32__

   char *assetId = "0";
   #endif
   
      #if defined __linux__ || __APPLE__

      snprintf(command, sizeof(command),
  "bash -c 'json_data=$(curl -s -H \"Accept: application/json\" \"%s\"); "
         "tag=$(echo \"$json_data\" | jq -r \".tag_name\"); "
         "name=$(echo \"$json_data\" | jq -r \".assets[%s].name\"); "
         "id=$(echo \"$json_data\" | jq -r \".assets[%s].id\"); "
         "if [ -z \"$tag\" ] || [ -z \"$name\" ] || [ \"$tag\" = \"null\" ] || [ \"$id\" = \"null\" ]  || [ \"$name\" = \"null\" ]; then exit 1; fi; "
         "url=\"%s$tag/$name\"; "
         "echo \"$url\" > \"%s\";"
         "echo \"$id\"  > \"%s\"'",
         githubUrls[0], assetId, assetId, githubUrls[1], 
         downloaderDirs[0], downloaderDirs[1]);
   
   #elif defined __WIN32__
    snprintf(command, sizeof(command),
      "powershell -Command \"$response = (Invoke-WebRequest -Uri '%s' -Headers @{Accept='application/json'}).Content | ConvertFrom-Json; "
      "$tag  = $response.tag_name;"
      "$name = $response.assets[%s].name;"
      "$id   = $response.assets[%s].id;"
      "$url  = '%s' + $tag + '/' + $name; "
      "[System.IO.File]::WriteAllText('%s', $url, [System.Text.Encoding]::ASCII); "
      "[System.IO.File]::WriteAllText('%s', $id , [System.Text.Encoding]::ASCII); \"", 
      githubUrls[0], assetId, assetId, githubUrls[1], 
      downloaderDirs[0], downloaderDirs[1]);
   #endif

   if (system(command) != 0) {
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

   #ifdef __linux__
   snprintf(command, sizeof(command), 
   "wget -O %s/xenia.zip %s", Paths[0], url);
   
   #elif defined __APPLE__
   snprintf(command, sizeof(command), "wget -O %s/xenia.zip %s", Paths[0], url);
   
   #elif defined __WIN32__
      snprintf(command, sizeof(command),"powershell -Command \"Invoke-WebRequest -Uri '%s' -OutFile '%s\\xenia.zip'\"", url, Paths[0]);

   #endif
   
   if (system(command) != 0) {
         log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to download emulator, aborting.\n");
         return false;
   }

   log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Success.\n");
   return true;
}

// If it's not the first download, fetch newVersion ID.
static bool updater(char **Paths, char **downloaderDirs, char **githubUrls)
{
   char url[260] = {0}, currentVersion[32] = {0}, newVersion[32] = {0}, command[1024] = {0};

   #ifdef __linux__

   char *assetId = "0";

   #elif defined __APPLE__

   char *assetId = "0";

   #elif defined __WIN32__

   char *assetId = "0";
   #endif
   
   #if defined __linux__ || __APPLE__
      snprintf(command, sizeof(command),
       "bash -c 'json_data=$(curl -s -H \"Accept: application/json\" \"%s\"); "
               "tag=$(echo \"$json_data\" | jq -r \".tag_name\"); "
               "name=$(echo \"$json_data\" | jq -r \".assets[%s].name\"); "
               "id=$(echo \"$json_data\" | jq -r \".assets[%s].id\"); "
               "if [ -z \"$tag\" ] || [ -z \"$name\" ] || [ \"$tag\" = \"null\" ] || [ \"$id\" = \"null\" ]  || [ \"$name\" = \"null\" ]; then exit 1; fi; "
               "url=\"%s$tag/$name\"; "
               "echo \"$url\" > \"%s\";"
               "echo \"$id\"  > \"%s\"'",
               githubUrls[0], assetId, assetId, githubUrls[1], downloaderDirs[0], downloaderDirs[2]);
   #elif defined __WIN32__
   snprintf(command, sizeof(command),
               "powershell -Command \"$response = (Invoke-WebRequest -Uri '%s' -Headers @{Accept='application/json'}).Content | ConvertFrom-Json; "
               "$tag  = $response.tag_name;"
               "$name = $response.assets[%s].name;"
               "$id   = $response.assets[%s].id;"
               "$url  = '%s' + $tag + '/' + $name; "
               "[System.IO.File]::WriteAllText('%s', $url, [System.Text.Encoding]::ASCII); "
               "[System.IO.File]::WriteAllText('%s', $id, [System.Text.Encoding]::ASCII); \"", 
               githubUrls[0], assetId, assetId, githubUrls[1], downloaderDirs[0], downloaderDirs[2]);
   #endif
         
      if (system(command) != 0) {
         log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to fetch update, aborting.\n");
         return false;
      } else { // Extract URL, currentID and newID for comparison
         FILE *urlFile = fopen(downloaderDirs[0], "r");
         FILE *currentVersionFile = fopen(downloaderDirs[1], "r");
         FILE *newVersionFile = fopen(downloaderDirs[2], "r");
         
         if (!urlFile || !currentVersionFile || !newVersionFile) {
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

               #ifdef __linux__

               snprintf(command, sizeof(command), 
               "wget -O %s/xenia.zip %s && chmod +x %s", Paths[0], url, Paths[6]);
               
               #elif defined __APPLE__
               
               snprintf(command, sizeof(command), 
                       "wget -O %s/xenia.zip %s", Paths[0], url);
               
               #elif defined __WIN32__
               
                snprintf(command, sizeof(command),
                       "powershell -Command \"Invoke-WebRequest -Uri '%s' -OutFile '%s\\xenia.zip'\"", url, Paths[0]);
               #endif

               if (system(command) != 0) {
                  log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to download update, aborting.\n");
                  return false;
               } else {
                  // Overwrite Current version.txt file with new ID if download was successfull.
                  #if defined __linux__ || __APPLE__
                  
                  snprintf(command, sizeof(command),
                  "bash -c 'json_data=$(curl -s -H \"Accept: application/json\" \"%s\"); "
                        "id=$(echo \"$json_data\" | jq -r \".assets[%s].id\"); "
                        "if [ \"$id\" = \"null\" ]; then exit 1; fi; "
                        "url=\"%s$tag/$name\"; "
                        "echo \"$id\"  > \"%s\"'",
                        githubUrls[0], assetId, githubUrls[1], downloaderDirs[1]);

                  #elif defined __WIN32__
                  
                  snprintf(command, sizeof(command),
                           "powershell -Command \"$response = (Invoke-WebRequest -Uri '%s' -Headers @{Accept='application/json'}).Content | ConvertFrom-Json; "
                           "$id   = $response.assets[%s].id;"
                           "[System.IO.File]::WriteAllText('%s', $id, [System.Text.Encoding]::ASCII); \"", 
                           githubUrls[0], assetId, downloaderDirs[1]);

                  #endif
               
                  if (system(command) != 0) {
                     log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to update current version file. Aborting.\n");
                     return false;
                  } else {
                     log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Success.\n");
                     return true;
                  }
               }
            } else {
               log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: No update found.\n");
            }
         }
      }
   return false;
}

static bool extractor(char **dirs)
{
   char command[1024] = {0};
   
   #ifdef __WIN32__
   
   snprintf(command, sizeof(command),
            "powershell -Command \"Expand-Archive -Path '%s\\xenia.zip' -DestinationPath '%s' -Force; Remove-Item -Path '%s\\xenia.zip' -Force\"", 
            dirs[0], dirs[0], dirs[0]);
   #elif defined __APPLE__ || __linux__
   snprintf(command, sizeof(command), 
   "mkdir %s/tmp_dir && "
           "unzip %s/xenia.zip -d %s/tmp_dir && " 
           "mv %s/tmp_dir/* %s && " \
           "rm -rf %s/tmp_dir && "
           "rm %s/xenia.zip && "
           "chmod +x %s", 
           dirs[0], dirs[0], dirs[0], dirs[0], 
           dirs[0], dirs[0], dirs[0], dirs[6]);
   #endif             

   if (system(command) != 0) {
      log_cb(RETRO_LOG_ERROR,"[LAUNCHER-ERROR]: Failed to extract emulator, aborting.\n");
      return false;
   } else {
      log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Success.\n");
      return true;
   }
}

static bool patchDownloader(char **Paths, char **githubUrls)
{
   char command[1024] = {0};

   #if defined __linux__ || __APPLE__
   snprintf(command, sizeof(command),  
"wget -O %s/patches.zip %s && "
       "unzip %s/patches.zip -d %s && "
       "rm -rf %s/patches.zip", 
       Paths[0], githubUrls[2], Paths[0], Paths[0],
       Paths[0]);

   #elif defined __WIN32__
   snprintf(command, sizeof(command),
      "powershell -Command \"Invoke-WebRequest -Uri '%s' -OutFile '%s\\patches.zip';"
      "Expand-Archive -Path '%s\\patches.zip' -DestinationPath '%s' -Force; Remove-Item -Path '%s\\patches.zip' -Force\"", 
      githubUrls[2], Paths[0], Paths[0], Paths[0], Paths[0]);
   #endif

   if (system(command) != 0) {
      log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to download patches, aborting.\n");
      return false;
   }
   return true;
}

/**
 * libretro callback; Called when a game is to be loaded.
*  - attach ROM absolute path from info->path in double quotes for system() function, avoids truncation.
*  - if info->path has no ROM, fallback to bios file placed by the user.
*/
bool retro_load_game(const struct retro_game_info *info)
{

   // Default Emulator Paths
   #ifdef __linux__

   const char *home = getenv("HOME");

   char *dirs[] = {
         "/.config/retroarch/system/xenia",
         "/.config/retroarch/system/xenia/bios",
         "/.config/retroarch/thumbnails/Microsoft - Xbox 360",
         "/.config/retroarch/thumbnails/Microsoft - Xbox 360/Named_Boxarts",
         "/.config/retroarch/thumbnails/Microsoft - Xbox 360/Named_Snaps",
         "/.config/retroarch/thumbnails/Microsoft - Xbox 360/Named_Titles",
         "/.config/retroarch/system/xenia/xenia_canary_netplay.exe" // search Path for glob.
      };
   
   // Emulator build versions and URL to download. Content is generated from powershell cmds
   char *downloaderDirs[] = {
      "/.config/retroarch/system/xenia/0.Url.txt",
      "/.config/retroarch/system/xenia/1.CurrentVersion.txt",
      "/.config/retroarch/system/xenia/2.NewVersion.txt",
   };

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

   #elif defined __APPLE__

   const char *home = getenv("HOME");

   char *dirs[] = {
         "/Library/Application Support/RetroArch/system/xenia",
         "/Library/Application Support/RetroArch/system/xenia/bios",
         "/Library/Application Support/RetroArch/thumbnails/Microsoft - Xbox 360",
         "/Library/Application Support/RetroArch/thumbnails/Microsoft - Xbox 360/Named_Boxarts",
         "/Library/Application Support/RetroArch/thumbnails/Microsoft - Xbox 360/Named_Snaps",
         "/Library/Application Support/RetroArch/thumbnails/Microsoft - Xbox 360/Named_Titles",
         "/Library/Application Support/RetroArch/system/xenia/xenia_canary_netplay.exe" // search Path for glob.
      };

   // Emulator build versions and URL to download. Content is generated from powershell cmds
   char *downloaderDirs[] = {
      "/Library/Application Support/RetroArch/system/xenia/0.Url.txt",
      "/Library/Application Support/RetroArch/system/xenia/1.CurrentVersion.txt",
      "/Library/Application Support/RetroArch/system/xenia/2.NewVersion.txt",
   };

   size_t numPaths = sizeof(dirs)/sizeof(char*);
   size_t dirPaths = sizeof(downloaderDirs)/sizeof(char*);

   for (size_t i = 0; i < numPaths; i++) {
      char *tmp = dirs[i];
      asprintf(&dirs[i], "%s%s", home, tmp );
   }

   for (size_t i = 0; i < dirPaths; i++) {
      char *tmp = downloaderDirs[i];
      asprintf(&downloaderDirs[i], "%s%s", home, tmp );
   }

   #elif defined __WIN32__

   char *dirs[] = {
         "C:\\RetroArch-Win64\\system\\xenia",
         "C:\\RetroArch-Win64\\system\\xenia\\bios",
         "C:\\RetroArch-Win64\\thumbnails\\Microsoft - Xbox 360",
         "C:\\RetroArch-Win64\\thumbnails\\Microsoft - Xbox 360\\Named_Boxarts",
         "C:\\RetroArch-Win64\\thumbnails\\Microsoft - Xbox 360\\Named_Snaps",
         "C:\\RetroArch-Win64\\thumbnails\\Microsoft - Xbox 360\\Named_Titles",
         "C:\\RetroArch-Win64\\system\\xenia\\xenia_canary_netplay.exe" //searchPath
      };
   
   char *downloaderDirs[] = {
      "C:\\RetroArch-Win64\\system\\xenia\\0.Url.txt",
      "C:\\RetroArch-Win64\\system\\xenia\\1.CurrentVersion.txt",
      "C:\\RetroArch-Win64\\system\\xenia\\2.NewVersion.txt",
   };

   size_t numPaths = sizeof(dirs)/sizeof(char*);

   #endif

   char *githubUrls[] = {
      "https://api.github.com/repos/AdrianCassar/xenia-canary/releases/latest",
      "https://github.com/AdrianCassar/xenia-canary/releases/download/",
      "https://github.com/xenia-canary/game-patches/releases/latest/download/game-patches.zip"
   };

   char executable[513] = {0};

   /**
    * If no emulator was found, download and extract it
    * If emulator was found check for updates and extract them.
    */

    #if defined __linux__ || __APPLE__
    
    if (system("wineboot") != 0 || system("winetricks --force dxvk vkd3d") != 0) {
      log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: wine and winetricks are needed to run xenia netplay.\n");
      return false;
    }
    #endif

   if (!setup(dirs, numPaths, executable)) {
      if (downloader(dirs, downloaderDirs, githubUrls)) {
         if (extractor(dirs)) {
            setup(dirs, numPaths, executable);
            patchDownloader(dirs, githubUrls);
         }
      }
   } else {
      if (updater(dirs, downloaderDirs, githubUrls)) {
         if (extractor(dirs)) {
            patchDownloader(dirs, githubUrls);
         }
      }
   }

   // if executable exists, only then try to launch it.
   if (strlen(executable) > 0) {
      if (info != NULL && info->path != NULL) {
         char args[512] = {0};
         snprintf(args, sizeof(args), " --fullscreen=true \"%s\"", info->path);
         strncat(executable, args, sizeof(executable)-1);
      }

      if (system(executable) == 0) {
         log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Finished running Xenia.\n");
         return true;
      } else {
         log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed running Xenia.\n");
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
