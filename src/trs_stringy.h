int stringy;

void stringy_init(void);
const char *stringy_get_name(int unit);
int stringy_insert(int unit, const char *name);
void stringy_remove(int unit);
int stringy_create(const char *name);
int stringy_in(int unit);
void stringy_out(int unit, int value);
void stringy_reset(void);
void stringy_change_all(void);
