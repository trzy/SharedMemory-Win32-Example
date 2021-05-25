#include <windows.h>
#include <iostream>
#include <conio.h>
#include <cstdint>
#include <tchar.h>

static const uint32_t k_shared_region_size = 256;
static const TCHAR k_shared_region_name[] = TEXT("Local\\MyFileMappingObject");

int main(int argc, char **argv)
{
  // Get event handles
  ///HANDLE writer_event_handle = CreateEventA(NULL, false, false, "Local\\WriterEvent"); // This works too
  HANDLE writer_event_handle = OpenEventA(EVENT_MODIFY_STATE, false, "Local\\WriterEvent");
  if (writer_event_handle == NULL)
  {
    std::cerr << "Unable to open event object: code=" << GetLastError() << std::endl;
    return 1;
  }
  HANDLE reader_event_handle = OpenEventA(EVENT_MODIFY_STATE, false, "Local\\ReaderEvent");
  if (reader_event_handle == NULL)
  {
    std::cerr << "Unable to open event object: code=" << GetLastError() << std::endl;
    CloseHandle(writer_event_handle);
    return 1;
  }

  // Get shared memory
  HANDLE map_fp = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, k_shared_region_name);
  if (map_fp == NULL)
  {
    std::cerr << "Unable to open mapping object: code=" << GetLastError() << std::endl;
    CloseHandle(writer_event_handle);
    CloseHandle(reader_event_handle);
    return 1;
  }
  const char *buffer = (const char *) MapViewOfFile(map_fp, FILE_MAP_ALL_ACCESS, 0, 0, k_shared_region_size);
  if (!buffer)
  {
      std::cerr << "Unable to get pointer to file view" << std::endl;
      CloseHandle(map_fp);
      CloseHandle(writer_event_handle);
      CloseHandle(reader_event_handle);
      return 1;
  }

  // Continuously read value and print whenever it changes
  uint32_t prev_value = 0;
  while (true)
  {
    WaitForSingleObject(writer_event_handle, INFINITE);
    uint32_t value = *((uint32_t *) buffer);
    if (value != prev_value)
    {
      std::cout << value << std::endl;
      prev_value = value;
    }
    SetEvent(reader_event_handle);
  }

  UnmapViewOfFile(buffer);
  CloseHandle(map_fp);
  CloseHandle(writer_event_handle);
  CloseHandle(reader_event_handle);

  return 0;
}