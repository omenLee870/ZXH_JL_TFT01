#include "bitstore.h"
#include "system/includes.h"


struct database {
    OS_MUTEX mutex;
    u16 table_num;
    u16 value_len;
    u32 *tables_value;
    const struct db_table *tables;
};

static struct database db_hdl;

#define    VM_DATABASE_INDEX                 3
#define db (&db_hdl)

static int __db_store(void *data, int len)
{
    os_mutex_pend(&db->mutex, 0);

    *(u16 *)((u8 *)data + len) = CRC16(data, len);
    if (syscfg_write(VM_DATABASE_INDEX, data, len + 2) != len + 2) {
        len = -EFAULT;
    }

    os_mutex_post(&db->mutex);
    printf("__db_store  succ len:%d\n", len);
    return len;
}

static int __db_restore(void *data, int len)
{
    u16 crc;

    os_mutex_pend(&db->mutex, 0);

    if (len + 2 != syscfg_read(VM_DATABASE_INDEX, data, len + 2)) {
        len = 0;
    } else {
        crc = CRC16(data, len);
        if (crc != *(u16 *)((u8 *)data + len)) {
            len = 0;
        }
    }

    printf("__db_restore  succ len:%d\n", len);
    os_mutex_post(&db->mutex);

    return len;
}

static u32 __get_bits(u32 w, int offset, int bits)
{
    w >>= offset;
    w &= (1 << bits) - 1;

    return w;
}

static void __set_bits(u32 *w, int offset, int bits, u32 value)
{
    int m = (1 << bits) - 1;

    value = (value & m) << offset;
    *w &= ~(m << offset);
    *w ^= value;
}

static void get_bits(int offset, int bits, u32 *value)
{
    int a, b, c, n;

    a = offset / 32;
    b = offset & 31;
    c = 32 - b;

    os_mutex_pend(&db->mutex, 0);

    if (bits <= 32) {
        n = bits < c ? bits : c;
        *value = __get_bits(db->tables_value[a], b, n);
        bits -= n;
        if (bits) {
            *value |= __get_bits(db->tables_value[++a], 0, bits) << n;
        }
    }

    os_mutex_post(&db->mutex);
}



static void set_bits(int offset, int bits, u32 value)
{
    int a, b, c, n;

    a = offset / 32;
    b = offset & 31;
    c = 32 - b;

    os_mutex_pend(&db->mutex, 0);

    if (bits <= 32) {
        n = bits < c ? bits : c;
        __set_bits(&db->tables_value[a], b, n, value);
        bits -= n;
        if (bits) {
            __set_bits(&db->tables_value[++a], 0, bits, value >> n);
        }
    } else {

    }

    os_mutex_post(&db->mutex);
}

int db_flush()
{
    return __db_store(db->tables_value, db->value_len);
}

int db_reset()
{
    int i;
    int bits = 0;

    for (i = 0, bits = 0; i < db->table_num; i++) {
        set_bits(bits, db->tables[i].value_bits, db->tables[i].value);
        bits += db->tables[i].value_bits;
    }

    return __db_store(db->tables_value, db->value_len);
}

u32 db_select(const char *table)
{
    int i;
    int b;
    int bits = 0;
    u32 value;

    for (i = 0; i < db->table_num; i++) {
        b = db->tables[i].value_bits;

        if (!strcmp(table, db->tables[i].name)) {
            get_bits(bits, b, &value);
            return value;
        }

        bits += b;
    }


    return -EINVAL;
}

int db_update(const char *table, u32 value)
{
    int i;
    int b;
    int bits = 0;

    for (i = 0; i < db->table_num; i++) {
        b = db->tables[i].value_bits;

        if (!strcmp(table, db->tables[i].name)) {
            set_bits(bits, b, value);
            __db_store(db->tables_value, db->value_len);
            return 0;
        }
        bits += b;
    }

    return -EINVAL;
}

int db_create_table(const struct db_table *table, int num)
{
    int i;
    int byte;
    int b, bits = 0;

    db->tables = table;
    db->table_num = num;

    for (i = 0; i < db->table_num; i++) {
        bits += db->tables[i].value_bits;
    }

    byte = (bits + 31) / 32 * 4;

    db->tables_value = (u32 *)malloc(byte + 2);
    if (!db->tables_value) {
        return -ENOMEM;
    }
    db->value_len = byte;

    if (__db_restore(db->tables_value, byte) != byte) {
        db_reset();
    }

    return 0;
}

int db_erase()
{
    printf(" err db_erase not define \n\n\n\n");
    return 0;//dev_ioctl(db->dev, DEV_VM_ERASE, 0);
}

int db_create()
{
    db->tables = NULL;
    db->table_num = 0;

    os_mutex_create(&db->mutex);

    return 0;
}





