
typedef  uint8_t    matrix_row_t;

#define MATRIX_IS_ON(row, col)  (matrix_get_row(row) && (1<<col))


#ifdef __cplusplus
extern "C" {
#endif

uint8_t matrix_rows(void);
uint8_t matrix_cols(void);
/* should be called at early stage of startup before matrix_init.(optional) */
void matrix_setup(void);
/* intialize matrix for scaning. */
void matrix_init(void);
/* scan all key states on matrix */
uint8_t matrix_scan(void);
/* whether a swtich is on */
bool matrix_is_on(uint8_t row, uint8_t col);
matrix_row_t matrix_get_row(uint8_t row);
void matrix_print(void);
void matrix_clear(void);
/* power control */
void matrix_power_up(void);
void matrix_power_down(void);
void matrix_print(void);

#ifdef __cplusplus
}
#endif
