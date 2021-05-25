#include <windows.h>
#include <iostream>
#include <conio.h>
#include <cstdint>
#include <tchar.h>

static const uint32_t k_shared_region_size = 256;
static const TCHAR k_shared_region_name[] = TEXT("Local\\MyFileMappingObject");

int main(int argc, char **argv)
{
  // Create two events: one that reader waits on and writer signals, and one that writer waits on and reader signals
  HANDLE writer_event_handle = CreateEventA(NULL, false, false, "Local\\WriterEvent");
  if (!writer_event_handle)
  {
    std::cerr << "Unable to create event: code=" << GetLastError() << std::endl;
    return 1;
  }
  HANDLE reader_event_handle = CreateEventA(NULL, false, false, "Local\\ReaderEvent");
  if (!reader_event_handle)
  {
    std::cerr << "Unable to create event: code=" << GetLastError() << std::endl;
    CloseHandle(writer_event_handle);
  }

  // Create shared memory region and get pointer to it
  HANDLE map_fp = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, k_shared_region_size, k_shared_region_name);
  if (!map_fp)
  {
    std::cerr << "Unable to create mapping object: code=" << GetLastError() << std::endl;
    CloseHandle(writer_event_handle);
    CloseHandle(reader_event_handle);
    return 1;
  }
  uint8_t *buffer = (uint8_t *) MapViewOfFile(map_fp, FILE_MAP_ALL_ACCESS, 0, 0, k_shared_region_size);
  if (!buffer)
  {
      std::cerr << "Unable to get pointer to file view" << std::endl;
      CloseHandle(map_fp);
      CloseHandle(writer_event_handle);
      CloseHandle(reader_event_handle);
      return 1;
  }

  // Continuously increment first integer in buffer using writer event to signal to reader and
  // waiting on reader event before repeating
  uint32_t value = 0;
  while (true)
  {
    getch();

    *((uint32_t *) buffer) = value++;
    std::cout << "Wrote " << (value - 1) << std::endl;

    // Signal that reader may procede
    SetEvent(writer_event_handle);

    // Wait for reader to finish so we can write again
    WaitForSingleObject(reader_event_handle, INFINITE);
  }

  UnmapViewOfFile(buffer);
  CloseHandle(map_fp);
  CloseHandle(writer_event_handle);
  CloseHandle(reader_event_handle);
  return 0;
}
