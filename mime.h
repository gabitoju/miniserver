#ifndef MIME
#define MIME

void load_mime_database();
void mime_destroy();
const char* file_mime_type(char* file_path);

#endif // MIME
