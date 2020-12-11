int put(char *key, char *value, char *error, int sock_fd);
int get(char *key, char *value, char *error, int sock_fd);
int del(char *key, char *error, int sock_fd);
void show_cache_data(int sock_fd);
void show_file_db(int sock_fd);