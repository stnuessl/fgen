/*
 * Copyright (C) 2018  Steffen Nüssle
 * fgen - method generator
 *
 * This file is part of fgen.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

struct s1 {
    int a, b, c;
};

void s1_set_a(struct s1 *s1, int value);
void s1_set_b(struct s1 *s1, int value);
void s1_set_c(struct s1 *s1, int value);

int s1_a(const struct s1 *s1);
int s1_b(const struct s1 *s1);
int s1_c(const struct s1 *s1);

struct s2 {
    long a_, b_, c_;
};

void s2_set_a(struct s2 *, long);
void s2_set_b(struct s2 *s2, long);
void s2_set_c(struct s2 *, long value);

long s2_get_a(const struct s2 *s2);
long s2_get_b(const struct s2 *);
long s2_get_c(const struct s2 *self);

struct S3 {
    double AA, AB, BB; 
};

void S3setAA(struct S3 *S3, double value);
void S3setAB(struct S3 *S3, double value);
void S3setBB(struct S3 *S3, double value);

double S3GetAA(const struct S3 *S3);
double S3GetAB(const struct S3 *S3);
double S3GetBB(const struct S3 *S3);

struct S4 {
    int A_, B_;
};

int s4GetA(const struct S4 *S4);
int s4GetB(const struct S4 *);

void s4SetA(struct S4 *S4, int value);
void s4SetB(struct S4 *, int);

struct s5 {
    int abc, bac, bca, acb, cab, cba;
};

typedef s5 s5;

int s5_abc(const struct s5 *);
int s5_bac(const struct s5 *);
int s5_bca(const struct s5 *);
int s5_acb(const s5 *);
int s5_cab(const s5 *);
int s5_cba(const s5 *);

void s5_set_abc(struct s5 *s5, int value);
void s5_set_bac(struct s5 *, int value);
void s5_set_bca(struct s5 *s5, int);
void s5_set_acb(s5 *s5, int value);
void s5_set_cab(s5 *, int value);
void s5_set_cba(s5 *s5, int);

struct s6 {
    int A_, B_;
};

