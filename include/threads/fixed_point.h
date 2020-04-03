#define INT_MAX ((1 << 31) - 1)
#define INT_MIN (-(1 << 31))
#define f (1 << 14)

int to_f(int n);
int to_n(int x);
int to_nr(int x);
int add_ff(int x, int y);
int sub_ff(int x, int y);
int add_fn(int x, int n);
int sub_fn(int x, int n);
int mul_ff(int x, int y);
int mul_fn(int x, int n);
int div_ff(int x, int y);
int div_fn(int x, int n);

int to_f(int n){
	return n * f;
}
int to_n(int x){
	return x / f;
}
int to_nr(int x){
	return x>=0 ? (x+f/2)/f : (x-f/2)/f;
}
int add_ff(int x, int y){
	return x + y;
}
int sub_ff(int x, int y){
	return x - y;
}
int add_fn(int x, int n){
	return x + n * f;
}
int sub_fn(int x, int n){
	return x - n * f;
}
int mul_ff(int x, int y){
	return ((int64_t) x) * y / f;
}
int mul_fn(int x, int n){
	return x * n;
}
int div_ff(int x, int y){
	return ((int64_t) x) * f / y;
}
int div_fn(int x, int n){
	return x / n ;
}
