#ifndef MIME
#define MIME

void load_mime_database(char* mime_types_path);
void mime_destroy();
const char* file_mime_type(char* file_path);

#endif // MIME
