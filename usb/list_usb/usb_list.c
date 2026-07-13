/*
 * usb_list.c — List top-level contents of a USB drive
 *
 * Usage: ./usb_list <mount_point>
 *   e.g. ./usb_list /run/media/sda2
 *
 * Output is saved to USBContent.txt in the current directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

static void format_perms(mode_t mode, char *buf)
{
    buf[0] = S_ISDIR(mode) ? 'd' : '-';
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';
    buf[10] = '\0';
}

int main(int argc, char *argv[])
{
    const char *mount_point;
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char perms[11];
    int dir_count = 0, file_count = 0;
    FILE *out;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <mount_point>\n", argv[0]);
        fprintf(stderr, "  e.g. %s /run/media/sda2\n", argv[0]);
        return 1;
    }

    mount_point = argv[1];

    dir = opendir(mount_point);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    out = fopen("USBContent.txt", "w");
    if (!out) {
        perror("fopen USBContent.txt");
        closedir(dir);
        return 1;
    }

    fprintf(out, "USB Drive: %s\n", mount_point);
    fprintf(out, "========================================\n");

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 mount_point, entry->d_name);

        if (stat(full_path, &file_stat) < 0)
            continue;

        format_perms(file_stat.st_mode, perms);

        struct tm *tm = localtime(&file_stat.st_mtime);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm);

        fprintf(out, "%s %3lu %s %s %8ld %s %s\n",
                perms,
                (unsigned long)file_stat.st_nlink,
                "root",
                "root",
                (long)file_stat.st_size,
                time_str,
                entry->d_name);

        if (S_ISDIR(file_stat.st_mode))
            dir_count++;
        else
            file_count++;
    }

    fprintf(out, "\nSubdirectories: %d\n", dir_count);
    fprintf(out, "Files: %d\n", file_count);

    printf("Done! Output saved to USBContent.txt\n");
    printf("USBContent.txt:\n");
    fflush(out);
    rewind(out);
    char buf[256];
    while (fgets(buf, sizeof(buf), out))
        fputs(buf, stdout);

    fclose(out);
    closedir(dir);
    return 0;
}
