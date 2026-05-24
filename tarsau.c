#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_DOSYA 32
#define MAX_BOYUT (200LL * 1024 * 1024)

void usage() {
    printf("Kullanim:\n");
    printf("  tarsau -b dosya1 dosya2 ... -o arsiv.sau\n");
    printf("  tarsau -a arsiv.sau [dizin]\n");
}

int metin_mi(const char *dosya_adi) {
    FILE *f = fopen(dosya_adi, "rb");
    if (!f) return 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == 0) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

long dosya_boyutu(const char *dosya_adi) {
    struct stat st;
    if (stat(dosya_adi, &st) != 0) return -1;
    return st.st_size;
}

// Hem - hem de – (em dash) kabul eder
int mod_mi(const char *arg, const char *mod) {
    if (strcmp(arg, mod) == 0) return 1;
    // em dash UTF-8: 0xE2 0x80 0x93
    if (arg[0] == (char)0xE2 && arg[1] == (char)0x80 && arg[2] == (char)0x93) {
        if (strcmp(arg + 3, mod + 1) == 0) return 1;
    }
    return 0;
}

void ac(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Hata: Arsiv dosyasi belirtilmedi!\n");
        exit(1);
    }

    char *arsiv_adi = argv[2];
    char *dizin = (argc >= 4) ? argv[3] : ".";

    FILE *arsiv = fopen(arsiv_adi, "rb");
    if (!arsiv) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        exit(1);
    }

    char *ext = strrchr(arsiv_adi, '.');
    if (!ext || strcmp(ext, ".sau") != 0) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(arsiv);
        exit(1);
    }

    char ilk10[11] = {0};
    fread(ilk10, 1, 10, arsiv);
    int header_boyutu = atoi(ilk10);

    if (header_boyutu <= 0) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(arsiv);
        exit(1);
    }

    char *header = malloc(header_boyutu + 1);
    if (!header) {
        printf("Hata: Bellek ayrilamadi!\n");
        fclose(arsiv);
        exit(1);
    }
    fread(header, 1, header_boyutu, arsiv);
    header[header_boyutu] = '\0';

    struct stat st;
    if (stat(dizin, &st) != 0) {
        mkdir(dizin, 0755);
    }

    // Dosya isimlerini topla (mesaj icin)
    char isimler[MAX_DOSYA][256];
    int isim_sayisi = 0;

    char *p = header;
    while (*p == '|') p++;

    while (*p != '\0') {
        char *virgul1 = strchr(p, ',');
        if (!virgul1) break;
        *virgul1 = '\0';
        char dosya_adi[256];
        strncpy(dosya_adi, p, 255);
        dosya_adi[255] = '\0';
        strncpy(isimler[isim_sayisi++], dosya_adi, 255);
        p = virgul1 + 1;

        char *virgul2 = strchr(p, ',');
        if (!virgul2) break;
        *virgul2 = '\0';
        int izinler = (int)strtol(p, NULL, 8);
        p = virgul2 + 1;

        char *bitis = strchr(p, '|');
        if (bitis) *bitis = '\0';
        long boyut = atol(p);
        if (bitis) p = bitis + 1;
        else p += strlen(p);

        char tam_yol[512];
        snprintf(tam_yol, sizeof(tam_yol), "%s/%s", dizin, dosya_adi);

        FILE *cikti = fopen(tam_yol, "wb");
        if (!cikti) {
            printf("Hata: %s dosyasi olusturulamadi!\n", tam_yol);
            fclose(arsiv);
            free(header);
            exit(1);
        }

        char buf[4096];
        long kalan = boyut;
        while (kalan > 0) {
            int okuma = (kalan > 4096) ? 4096 : (int)kalan;
            fread(buf, 1, okuma, arsiv);
            fwrite(buf, 1, okuma, cikti);
            kalan -= okuma;
        }
        fclose(cikti);
        chmod(tam_yol, izinler);
    }

    // Cikti mesaji: "d1 dizininde t1, t2, t3 dosyalari acildi."
    printf("%s dizininde ", dizin);
    for (int i = 0; i < isim_sayisi; i++) {
        if (i > 0 && i == isim_sayisi - 1)
            printf(" ve ");
        else if (i > 0)
            printf(", ");
        printf("%s", isimler[i]);
    }
    printf(" dosyalari acildi.\n");

    free(header);
    fclose(arsiv);
}

void arsivle(int argc, char *argv[]) {
    char *dosyalar[MAX_DOSYA];
    int dosya_sayisi = 0;
    char *cikti = "a.sau";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                cikti = argv[i + 1];
                i++;
            }
        } else {
            if (dosya_sayisi >= MAX_DOSYA) {
                printf("Hata: En fazla %d dosya arsivlenebilir!\n", MAX_DOSYA);
                exit(1);
            }
            dosyalar[dosya_sayisi++] = argv[i];
        }
    }

    if (dosya_sayisi == 0) {
        printf("Hata: Hic dosya belirtilmedi!\n");
        exit(1);
    }

    long long toplam = 0;
    for (int i = 0; i < dosya_sayisi; i++) {
        if (!metin_mi(dosyalar[i])) {
            printf("%s giris dosyasinin formati uyumsuzdur!\n", dosyalar[i]);
            exit(1);
        }
        long b = dosya_boyutu(dosyalar[i]);
        if (b < 0) {
            printf("Hata: %s dosyasi okunamadi!\n", dosyalar[i]);
            exit(1);
        }
        toplam += b;
    }

    if (toplam > MAX_BOYUT) {
        printf("Hata: Toplam dosya boyutu 200 MB'i geciyor!\n");
        exit(1);
    }

    char header[1024 * 1024] = "";
    for (int i = 0; i < dosya_sayisi; i++) {
        struct stat st;
        stat(dosyalar[i], &st);
        char kayit[512];
        sprintf(kayit, "|%s,%o,%ld", dosyalar[i],
                (unsigned int)(st.st_mode & 0777), (long)st.st_size);
        strcat(header, kayit);
    }

    int header_boyutu = strlen(header);
    char ilk10[11];
    sprintf(ilk10, "%010d", header_boyutu);

    FILE *cikti_f = fopen(cikti, "wb");
    if (!cikti_f) {
        printf("Hata: %s dosyasi olusturulamadi!\n", cikti);
        exit(1);
    }

    fwrite(ilk10, 1, 10, cikti_f);
    fwrite(header, 1, header_boyutu, cikti_f);

    for (int i = 0; i < dosya_sayisi; i++) {
        FILE *f = fopen(dosyalar[i], "rb");
        if (!f) {
            printf("Hata: %s acilamadi!\n", dosyalar[i]);
            fclose(cikti_f);
            exit(1);
        }
        int c;
        while ((c = fgetc(f)) != EOF) {
            fputc(c, cikti_f);
        }
        fclose(f);
    }

    fclose(cikti_f);
    printf("Dosyalar birlestirildi.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    if (mod_mi(argv[1], "-b")) {
        arsivle(argc, argv);
    } else if (mod_mi(argv[1], "-a")) {
        ac(argc, argv);
    } else {
        usage();
        return 1;
    }

    return 0;
}