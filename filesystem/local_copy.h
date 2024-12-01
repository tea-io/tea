#ifndef LOCALCOPY_H
#define LOCALCOPY_H

void ensure_local_copy_initialized(int socket, const char *path);
void discard_local_copy(const char *path);

#endif // LOCALCOPY_H
