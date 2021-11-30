#include "libc.h"

int shell_theme(int theme);

int main(int argc, char** argv) {
    
    if (argc != 2) {
        printf("Usage: color <DEFAULT|WINDOWS>\n");
        return -1;
    }
    
    char *theme = argv[1];

    if (!streq(theme, "default") && !streq(theme, "windows")) {
        printf("Usage: color <DEFAULT|WINDOWS>\n");
        return -1;
    }

    printf("Setting shell theme to %s\n", theme);
    shell_theme(streq(theme, "default") ? 0 : 1);

    return 0;
}