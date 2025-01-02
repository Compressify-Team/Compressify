#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <fftw3.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>

void print_progress(double percentage) {
    int barWidth = 70;
    printf("[");
    int pos = barWidth * percentage;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %d %%\r", (int)(percentage * 100.0));
    fflush(stdout);
}

void compress_audio(const char *input_file, const char *output_file) {
    // Open input file
    SF_INFO sfinfo;
    SNDFILE *infile = sf_open(input_file, SFM_READ, &sfinfo);
    if (!infile) {
        printf("Failed to open input file\n");
        return;
    }

    // Read audio data
    int num_samples = sfinfo.frames * sfinfo.channels;
    double *buffer = (double *)malloc(num_samples * sizeof(double));
    sf_read_double(infile, buffer, num_samples);

    // Perform FFT
    fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * num_samples);
    fftw_plan p = fftw_plan_dft_r2c_1d(num_samples, buffer, out, FFTW_ESTIMATE);
    fftw_execute(p);

    // Write compressed data to output file
    FILE *outfile = fopen(output_file, "wb");
    fwrite(&sfinfo, sizeof(SF_INFO), 1, outfile); // Write SF_INFO to output file
    fwrite(out, sizeof(fftw_complex), num_samples, outfile);

    // Calculate compression ratio
    long original_size = num_samples * sizeof(double);
    long compressed_size = sizeof(SF_INFO) + num_samples * sizeof(fftw_complex);
    double compression_ratio = (double)compressed_size / original_size * 100.0;
    printf("\nOriginal size: %ld bytes\n", original_size);
    printf("Compressed size: %ld bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n", compression_ratio);

    // Clean up
    fclose(outfile);
    fftw_destroy_plan(p);
    fftw_free(out);
    sf_close(infile);
    free(buffer);
}

void decompress_audio(const char *input_file, const char *output_file) {
    // Open input file
    FILE *infile = fopen(input_file, "rb");
    if (!infile) {
        printf("Failed to open input file\n");
        return;
    }

    // Read SF_INFO from input file
    SF_INFO sfinfo;
    fread(&sfinfo, sizeof(SF_INFO), 1, infile);

    // Read compressed data
    fseek(infile, 0, SEEK_END);
    long file_size = ftell(infile) - sizeof(SF_INFO);
    fseek(infile, sizeof(SF_INFO), SEEK_SET);
    fftw_complex *buffer = (fftw_complex *)fftw_malloc(file_size);
    fread(buffer, 1, file_size, infile);

    // Perform inverse FFT
    int num_samples = file_size / sizeof(fftw_complex);
    double *out = (double *)malloc(num_samples * sizeof(double));
    fftw_plan p = fftw_plan_dft_c2r_1d(num_samples, buffer, out, FFTW_ESTIMATE);
    fftw_execute(p);

    // Normalize the output from IFFT
    for (int i = 0; i < num_samples; i++) {
        out[i] /= num_samples;
    }

    // Write decompressed data to output file
    SNDFILE *outfile = sf_open(output_file, SFM_WRITE, &sfinfo);
    sf_write_double(outfile, out, num_samples);

    // Calculate decompressed size
    long decompressed_size = num_samples * sizeof(double);
    double decompression_ratio = (double)decompressed_size / file_size * 100.0;
    printf("\nDecompressed size: %ld bytes\n", decompressed_size);
    printf("Decompression ratio: %.2f%%\n", decompression_ratio);

    // Clean up
    sf_close(outfile);
    fftw_destroy_plan(p);
    fftw_free(buffer);
    fclose(infile);
    free(out);
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    int command;
    while (1) {
        printf("Available commands-> 1:COMPRESS 2:DECOMPRESS 0:EXIT\n");
        printf("請輸入指令: ");
        if (scanf("%d", &command) != 1) {
            printf("無效的指令\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        struct rusage usage;
        struct timeval start, end;
        gettimeofday(&start, NULL);

        if (command == 0) {
            break;
        } else if (command == 1) {
            char input_file[256], output_file[256];
            printf("請輸入輸入檔案名稱: ");
            scanf("%s", input_file);
            printf("請輸入輸出檔案名稱: ");
            scanf("%s", output_file);
            compress_audio(input_file, output_file);
        } else if (command == 2) {
            char input_file[256], output_file[256];
            printf("請輸入輸入檔案名稱: ");
            scanf("%s", input_file);
            printf("請輸入輸出檔案名稱: ");
            scanf("%s", output_file);
            decompress_audio(input_file, output_file);
        } else {
            printf("無效的指令\n");
        }

        gettimeofday(&end, NULL);
        getrusage(RUSAGE_SELF, &usage);

        double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        printf("Elapsed time: %.2f seconds\n", elapsed_time);
        printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
    }

    return 0;
}