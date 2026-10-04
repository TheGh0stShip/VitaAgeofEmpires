/* vita-elf-create appends the module's SCE data (module info, import and
   export tables) to the end of the first (code) LOAD segment and fails
   when the data segment starts right after it on the next page. Leave one
   page between the read-only sections and the first writable section so
   that space always exists, whatever the code size. */
SECTIONS
{
  . = . + 0x1000;
}
INSERT BEFORE .init_array;
