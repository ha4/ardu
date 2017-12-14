#define TYP_QUERY  0
#define TYP_PT100  1
#define TYP_T      2
#define TYP_K      3
#define TYP_J      4

int conv_type(int typ);
double convT2V(double T);
double convV2T(double V);
