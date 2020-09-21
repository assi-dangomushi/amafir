/*
   Copyright (c) 2015-2019, Amanogawa Audio Labo
   All rights reserved.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   */

// amafir.c  version 0.13

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <fftw3.h>

#define N (8192*2)
#define MAX_S 80

const char *filename[] = {"LF.txt" ,"RF.txt", "LS.txt", "RS.txt" ,"LB.txt" ,"RB.txt", "CF.txt"};
const char wisdomfile[] = "wisdom.data";

//channel mapping:hdmi_play.bin Default
enum {Lf,Rf,Lfe,Cf,Ls,Rs,Lb,Rb};
// alsa hdmi
//enum {Lf,Rf,Lb,Rb,Cf,Lfe,Ls,Rs};
// alsa Xonar U7
//enum {Lf,Rf,Cf,Lfe,Lb,Rb,Ls,Rs};

void init_fir(fftw_complex **fir)
{
  char data[MAX_S];
  FILE *fp;
  static double in[N*2] = {};
  fftw_complex *out;
  out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N+1));
  fftw_plan p;
  p = fftw_plan_dft_r2c_1d(N*2, in, out, FFTW_ESTIMATE); 

  for (int i = 0; i < 7; i++) {
    fp = fopen(filename[i], "r");
    if (fp == NULL) {
      fprintf(stderr, "%s Error!\n", filename[i]);
      for (int j = 0; j < (N+1); j++) {
        fir[i][j] = 0;
      }
      continue;
    }
    for (int j = N; j < (N*2); j++) {
      in[j] = 0;
    }
    for (int j = N; j < (N*2); j++) {
      if (fgets(data, MAX_S, fp) == NULL) {
        break;
      }
      in[j] = atof(data);
    }
    fclose(fp);
    fftw_execute(p);
    for (int j = 0; j < (N+1); j++) {
      fir[i][j] = out[j] / (N*2);
    }
  }
  // fir[6]:CF (L+R)/2 
  for (int j = 0; j < (N+1); j++) {
    fir[6][j] = fir[6][j] / 2;
  }
  fftw_free(out);
}

void printhelp(void) 
{ 
  char s[] = "\n" 
    "This program receive 2ch 32bit from stdin and output 8ch 32bit to stdout\n" 
    "Usage: amafir coefDirectory\n"
    "coef-files : LF.txt, RF.txt, LB,txt, RB.txt, LS.txt, RS,txt, CF.txt\n"
    "LF,LB,LS:Left channel,  RF,RB,RS:Right channel, CF:(L+R)/2\n";
  fprintf(stderr, "%s", s); 
  return; 
} 

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printhelp();
    return 1;
  }
  if (chdir(argv[1])) {
    printhelp();
    return 1;
  }

  //load wisdomdata
  FILE *fp;
  fp = fopen(wisdomfile, "r");
  if (fp != NULL) {
    fftw_import_wisdom_from_file(fp);  
    fclose(fp);
  }

  //init fir data
  fftw_complex *fir[7];
  for (int i = 0; i < 7; i++) {
    fir[i] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N+1));
  }
  init_fir(fir);

  //init plan1 
  static double in1[3][N*2] = {};
  fftw_complex  *out1[3];
  fftw_plan p1[2];
  for (int i = 0; i < 2; i++) {
    out1[i] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N+1));
    p1[i] = fftw_plan_dft_r2c_1d(N*2, in1[i], out1[i], FFTW_PATIENT);
  }
  out1[2] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N+1));

  //init plan2
  static double out2[7][N*2];
  fftw_complex  *in2[7];
  fftw_plan p2[7];
  for (int i = 0; i < 7; i++) {
    in2[i] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N+1));
    p2[i] = fftw_plan_dft_c2r_1d(N*2, in2[i], out2[i], FFTW_PATIENT);
  }

  //save wisdomdata
  fp = fopen(wisdomfile,"w");
  if (fp != NULL){
    fftw_export_wisdom_to_file(fp);  
    fclose(fp);
  }

  static int32_t inbuf[N][2] = {};
  static int32_t outbuf[N][8] = {};

  // main loop
  while (1) {
    for (int_fast32_t i = 0; i < N; i++) {
      in1[0][i] = inbuf[i][0];
      in1[1][i] = inbuf[i][1];
    }
    int ret_n;
    ret_n = fread(inbuf, sizeof(int32_t), N*2, stdin);
    if (ret_n != N*2) {
      break;
    }

    for (int_fast32_t i = 0; i < N; i++) {
      in1[0][i+N] = inbuf[i][0];
      in1[1][i+N] = inbuf[i][1];
    }
    for (int_fast8_t i = 0; i < 2; i++) {
      fftw_execute(p1[i]);
    }
    for (int_fast32_t i = 0; i < (N+1); i++) {
      out1[2][i] = out1[0][i]+out1[1][i];
    }
    for (int_fast32_t i = 0; i < (N+1); i++) {
      in2[0][i] = out1[0][i] * fir[0][i];
      in2[1][i] = out1[1][i] * fir[1][i];
      in2[2][i] = out1[0][i] * fir[2][i];
      in2[3][i] = out1[1][i] * fir[3][i];
      in2[4][i] = out1[0][i] * fir[4][i];
      in2[5][i] = out1[1][i] * fir[5][i];
      in2[6][i] = out1[2][i] * fir[6][i];
    }
    for (int_fast8_t i = 0; i < 7; i++) {
      fftw_execute(p2[i]);
    }
    for (int_fast32_t i = 0; i < N; i++) {
      outbuf[i][Lf] = out2[0][i];
      outbuf[i][Rf] = out2[1][i];
      outbuf[i][Ls] = out2[2][i];
      outbuf[i][Rs] = out2[3][i];
      outbuf[i][Lb] = out2[4][i];
      outbuf[i][Rb] = out2[5][i];
      outbuf[i][Cf] = out2[6][i];
    }
    fwrite(outbuf, sizeof(int32_t), N*8, stdout);
  }
  return 0;
}

