#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libretro.h"

#ifdef __WIN32__
   #include <windows.h>
#elif defined __linux__ || __APPLE__
   #define ELF_MAGIC "\x7F""ELF"
   #include <glob.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <unistd.h>
   #include <fcntl.h>
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

/*
   Linux/macOS: Check if file is ELF, then use it.
*/
 int is_elf_executable(const char *filename) {
    
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
 *
 *  - Linux/macOS:
 *        - resolve HOME path 
 *        - create dir for emulator files 
 *        - apply regex search with glob, filter by file and ELF executable
 *  
 *  - Windows:
 *       - create dir for emulator files and bios
 *       - search for .exe binary with name pattern.
 *
 *    
 * - Final Steps:
 *       - attach ROM absolute path from info->path in double quotes for system() function, avoids truncation.
 *        - if info->path has no ROM, fallback to bios file placed by the user.
 */
bool retro_load_game(const struct retro_game_info *info)
{
   #ifdef __linux__

      glob_t buf;
      struct stat path_stat;
      char executable[512] = {0};
      char path[512] = {0};
      const char *home = getenv("HOME");
      
      if (!home) {
         return false;
      }
      
      // Create emulator folder if it doesn't exist
      snprintf(path, sizeof(path), "%s/.config/retroarch/system/PPSSPP", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: emulator folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: emulator folder already exist\n");
      }

      // search for binary executable.
      char tmpList[512] = {0};

      snprintf(tmpList, sizeof(tmpList), "%s/.config/retroarch/system/PPSSPP/PPSSPP*", home);

      if (glob(tmpList, 0, NULL, &buf) == 0) {
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

      if (strlen(executable) == 0) {
         printf("[LAUNCHER-ERROR]: No executable found, aborting\n");
         return false;
      }

   #elif defined __WIN32__
      WIN32_FIND_DATA findFileData;
      HANDLE hFind;
      char executable[MAX_PATH] = {0};
      char path[256] = "C:\\RetroArch-Win64\\system\\PPSSPP";
      char searchPath[MAX_PATH] = {0};

       if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) {
         _mkdir(path);
          printf("[LAUNCHER-INFO]: emulator folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: emulator folder already exist\n");
      }

      snprintf(searchPath, MAX_PATH, "%s\\PPSSPP*.exe", path);
      hFind = FindFirstFile(searchPath, &findFileData);

      if (hFind == INVALID_HANDLE_VALUE) {
         printf("[LAUNCHER-ERROR]: No executable found, aborting.\n");
         return false;
      }
      
      snprintf(executable, MAX_PATH, "%s\\%s", path, findFileData.cFileName);
      FindClose(hFind);
   #elif defined __APPLE__
      
      glob_t buf;
      struct stat path_stat;
      char executable[512] = {0};
      char path[512] = {0};
      const char *home = getenv("HOME");
      
      if (!home) {
         return false;
      }
      
      // Create emulator folder if it doesn't exist
      snprintf(path, sizeof(path), "%s/Library/Application Support/RetroArch/system/PPSSPP", home);

      if (stat(path, &path_stat) != 0) {
         mkdir(path, 0755);
         printf("[LAUNCHER-INFO]: emulator folder created in %s\n", path);
      } else {
         printf("[LAUNCHER-INFO]: emulator folder already exist\n");
      }

      // search for binary executable.
      char tmpList[512] = {0};

      snprintf(tmpList, sizeof(tmpList), "%s/Library/Application Support/RetroArch/system/PPSSPP*", home);

      if (glob(tmpList, 0, NULL, &buf) == 0) {
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

      if (strlen(executable) == 0) {
         printf("[LAUNCHER-ERROR]: No executable found, aborting\n");
         return false;
      }
   #endif
   
   const char *args[] = {" ", "\"", info->path, "\""};
   size_t size = sizeof(args)/sizeof(char*);

   for (size_t i = 0; i < size; i++) {
    strncat(executable, args[i], strlen(args[i]));
   } 

    printf("[LAUNCHER-INFO]: PPSSPP path: %s\n", executable);

   if (system(executable) == 0) {
      printf("[LAUNCHER-INFO]: Finished running PPSSPP.\n");
      return true;
   }

   printf("[LAUNCHER-INFO]: Failed running PPSSPP.\n");
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
