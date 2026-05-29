// hexdump.c
#include <unistd.h>     // read(), write(), close()
#include <fcntl.h>      // open(), O_RDONLY
#include <stdint.h>     // uint64_t
#include <stddef.h>     // size_t
#include <errno.h>      // errno, EINTR

// 1行に表示するバイト数
// 一般的なhexdumpと同じく16バイト単位で表示
#define BUF_SIZE 16

// 文字列の長さを数える関数
static size_t str_len(const char *s)
{
    size_t len = 0;

    while (s[len] != '\0')
    {
        len++;
    }

    return len;
}

// 標準エラー出力に文字列を書き込む
static void write_err(const char *s)
{
    write(STDERR_FILENO, s, str_len(s));
}

// 標準出力に文字列を書き込む
static void write_str(const char *s)
{
    write(STDOUT_FILENO, s, str_len(s));
}

// 1バイトの値を2桁の16進数で出力する
static void write_hex_bytes(unsigned char byte)
{
    const char hex[] = "0123456789abcdef";
    char out[2];

    // 上位4ビットを16進数1桁に変換
    out[0] = hex[(byte >> 4) & 0x0f];

    // 下位4ビットを16進数1桁に変換
    out[1] = hex[byte & 0x0f];

    write(STDOUT_FILENO, out, 2);
}


// 現在のファイル位置を8桁の16進数で出力する
static void write_hex_offset(uint64_t offset)
{
    const char hex[] = "0123456789abcdef";
    char out[8];

    // 下位ビットから順に取り出して、文字列の後ろから詰める
    for (int i = 7; i >= 0; i--)
    {
        out[i] = hex[offset & 0x0f];
        offset >>= 4;
    }

    write(STDOUT_FILENO, out, 8);
}

// 1バイトをASCIIとして表示する
static void write_ascii(unsigned char byte)
{
    char c;

    if (byte >= 0x20 && byte <= 0x7e)
    {
        c = (char)byte;
    } else
    {
        c = '.';
    }

    write(STDOUT_FILENO, &c, 1);
}

// 16バイト分のデータを1行として出力する
static void dump_line(unsigned char *buf, ssize_t n, uint64_t offset)
{
    // 行頭にオフセットを表示する
    write_hex_offset(offset);
    write_str("  ");

    // 16進数部分を表示
    for (int i = 0; i < BUF_SIZE; i++)
    {
        if (i < n)
        {
            // 読み込んだデータがある場合は16進数で表示
            write_hex_bytes(buf[i]);
        } else 
        {
            // 最後の行でデータが足りない部分は空白で埋める
            write_str("  ");
        }

        write_str(" ");

        // 8バイトごとに少し空白を開けて見やすくする
        if (i == 7)
        {
            write_str(" ");
        }
    }

    // ASCII表示の開始
    write_str(" |");

    // 読み込んだバイト列をASCIIとして表示する
    for (int i = 0; i < n; i++)
    {
        write_ascii(buf[i]);
    }

    write_str("|\n");
}

int main(int argc, char **argv)
{
    // 引数が1つだけ渡されているか確認
    if (argc != 2)
    {
        write_err("usage: ./hexdump <file>\n");
        return 1;
    }

    // ファイルを読取り専用で開く
    int fd = open(argv[1], O_RDONLY);

    // open()が失敗すると-1が返る
    if (fd < 0)
    {
        write_err("error: failed to open file\n");
        return 1;
    }

    // read()で読み込んだデータを一時で気に入れるbuf
    unsigned char buf[BUF_SIZE];

    // 現在のファイル先頭からの位置
    uint64_t offset = 0;

    while (1)
    {
        // ファイルから最大16バイト読み込む
        ssize_t n = read(fd, buf, BUF_SIZE);

        // read()が失敗すると-1が返る
        if (n < 0)
        {
            write_err("error: failed to read file\n");
            close(fd);
            return 1;
        }

        if (n == 0)
        {
            break; // ファイルの終わりに達した
        }

        // 読み込んだデータを1行分表示する
        dump_line(buf, n, offset);

        // 次の行のためのオフセットを進める
        offset += (uint64_t)n;
    }

    close(fd);

    return 0;
}
