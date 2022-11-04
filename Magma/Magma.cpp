// Magma.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <string>


#define block_size 4

using namespace std;

//таблица перестановок
const int permutationTable[8][16] =
{
  {1,7,14,13,0,5,8,3,4,15,10,6,9,12,11,2},
  {8,14,2,5,6,9,1,12,15,4,11,0,13,10,3,7},
  {5,13,15,6,9,2,12,10,11,7,8,1,4,3,14,0},
  {7,15,5,10,8,1,6,13,0,9,3,14,11,4,2,12},
  {12,8,2,1,13,4,15,6,7,0,10,5,3,14,9,11},
  {11,3,5,8,2,15,10,13,14,1,7,4,12,9,6,0},
  {6,8,2,3,9,10,5,12,1,14,4,7,11,13,0,15},
  {12,4,6,2,10,5,11,9,14,8,13,7,0,3,15,1}
};


//перевод текста в числовой формат
long long text_to_num(string str)
{
    long long result = 0;
    for (int i = 0; i < str.length(); i++)
    {
        result <<= 8;
        cout << "result=" << result << endl;
        if (str[i] < 0)
        {
            result += str[i] + 256;
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

//перевод числа в текст
string num_to_text(long long num)
{
    string result = "";
    for (int i = 0; i < 8; i++)
    {
        result = (char)(num % 256) + result;
        num /= 256;
    }
    return result;
}



//развертывание ключей
string keyDeployment(string key, int num)
{
    string iter_key;
    int newNum;
    if (num >= 24)
    {
        newNum = num % 8;
        newNum = abs(newNum - 7);
    }
    else {
        newNum = num % 8;
    }
    newNum = newNum * 8;
    iter_key = key.substr(newNum, 8);

    return iter_key;
}

//сложение двух двочиных векторов по модулю 2
 void GOST_Magma_Add(const uint8_t* a, const uint8_t* b, uint8_t* c) 
{
    int i;
    for (i = 0; i < 4; i++)
        c[i] = a[i] ^ b[i];
}

 void Magma_T(const uint8_t * in_data, uint8_t * out_data)
 {
     uint8_t first_part_byte, sec_part_byte;
     int i;
     for (i = 0; i < 4; i++)
     {
         // Извлекаем первую 4-битную часть байта
         first_part_byte = (in_data[i] & 0xf0) >> 4;
         // Извлекаем вторую 4-битную часть байта
         sec_part_byte = (in_data[i] & 0x0f);
         // Выполняем замену в соответствии с таблицей подстановок
         first_part_byte = permutationTable[i * 2][first_part_byte];
         sec_part_byte = permutationTable[i * 2 + 1][sec_part_byte];
         // «Склеиваем» обе 4-битные части обратно в байт
         out_data[i] = (first_part_byte << 4) | sec_part_byte;
     }
 }

void Magma_Add_32(const uint8_t* a, uint8_t b, uint8_t* c)
 {
     int i;
     // / 2^8
     unsigned int internal = 0;
     int x=24, y=0;
     for (i = 3; i >= 0; i--)
     {
         internal = a[i] + (b/(2^x))%(2^y) + (internal >> 8);
         x -= 8;
         y += 8;
         c[i] = internal & 0xff;
     }
 }


//Преобразование g
//Это преобразование включает в себя сложение правой части блока с итерационным ключом по модулю 32, 
//нелинейное биективное преобразование и сдвиг влево на одиннадцать разрядов 
 void Magma_g(string k, const uint8_t* a, uint8_t* out_data)
 {
     uint8_t internal[4];
     uint32_t out_data_32;
     uint8_t new_key = text_to_num(k);
     // Складываем по модулю 32 правую половину блока с итерационным ключом
     Magma_Add_32(a, new_key, internal);
     // Производим нелинейное биективное преобразование результата
     Magma_T(internal, internal);
     // Преобразовываем четырехбайтный вектор в одно 32-битное число
     out_data_32 = internal[0];
     out_data_32 = (out_data_32 << 8) + internal[1];
     out_data_32 = (out_data_32 << 8) + internal[2];
     out_data_32 = (out_data_32 << 8) + internal[3];
     // Циклически сдвигаем все влево на 11 разрядов
     out_data_32 = (out_data_32 << 11) | (out_data_32 >> 21);
     // Преобразовываем 32-битный результат сдвига обратно в 4-байтовый вектор
     out_data[3] = out_data_32;
     out_data[2] = out_data_32 >> 8;
     out_data[1] = out_data_32 >> 16;
     out_data[0] = out_data_32 >> 24;
 }

void Magma_G(string  k, uint8_t * a, uint8_t * out_data)
 {
     uint8_t a_0[4]; // Правая половина блока
     uint8_t a_1[4]; // Левая половина блока
     uint8_t G[4];

     int i;
     // Делим 64-битный исходный блок на две части
     for (i = 0; i < 4; i++)
     {
         a_0[i] = a[4 + i];
         a_1[i] = a[i];
     }

     // Производим преобразование g
     Magma_g(k, a_0, G);
     // Ксорим результат преобразования g с левой половиной блока
     GOST_Magma_Add(a_1, G, G);

     for (i = 0; i < 4; i++)
     {
         // Пишем в левую половину значение из правой
         a_1[i] = a_0[i];
         // Пишем результат GOST_Magma_Add в правую половину блока
         a_0[i] = G[i];
     }

     // Сводим правую и левую части блока в одно целое
     for (i = 0; i < 4; i++)
     {
         out_data[i] = a_1[i];
         out_data[4 + i] = a_0[i];
     }
 }

static void Magma_G_Fin(string  k, const uint8_t * a, uint8_t * out_data)
{
    uint8_t a_0[4]; // Правая половина блока
    uint8_t a_1[4]; // Левая половина блока
    uint8_t G[4];

    int i;
    // Делим 64-битный исходный блок на две части
    for (i = 0; i < 4; i++)
    {
        a_0[i] = a[4 + i];
        a_1[i] = a[i];
    }

    // Производим преобразование g
    Magma_g(k, a_0, G);
    // Ксорим результат преобразования g с левой половиной блока
    GOST_Magma_Add(a_1, G, G);
    // Пишем результат GOST_Magma_Add в левую половину блока
    for (i = 0; i < 4; i++)
        a_1[i] = G[i];

    // Сводим правую и левую части блока в одно целое
    for (i = 0; i < 4; i++)
    {
        out_data[i] = a_1[i];
        out_data[4 + i] = a_0[i];
    }
}

void Magma_Encript(long long blk, int out_blk, string key)
{
    int i;
    // Первое преобразование G
    string iter_key = keyDeployment(key,0);
    Magma_G(iter_key, blk, out_blk);
    // Последующие (со второго по тридцать первое) преобразования G
    for (i = 1; i < 31; i++)
    {
        iter_key = keyDeployment(key, i);
        Magma_G(iter_key, out_blk, out_blk);
    }
    // Последнее (тридцать второе) преобразование G
    iter_key = keyDeployment(key, 31);
    Magma_G_Fin(iter_key, out_blk, out_blk);
}

/*void Magma_Decript(const uint8_t* blk, uint8_t* out_blk)
{
    int i;

    // Первое преобразование G с использованием
    // тридцать второго итерационного ключа
    Magma_G(iter_key[31], blk, out_blk);
    // Последующие (со второго по тридцать первое) преобразования G
    // (итерационные ключи идут в обратном порядке)
    for (i = 30; i > 0; i--)
        Magma_G(iter_key[i], out_blk, out_blk);
    // Последнее (тридцать второе) преобразование G
    // с использованием первого итерационного ключа
    Magma_G_Fin(iter_key[0], out_blk, out_blk);
}
*/
int main()
{
    setlocale(LC_ALL, "rus");
    string key = "ffeeddccbbaa99887766554433221100f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"; // ключ
    string openText = "fedcba9876543210"; //текст для шифрования
    cout <<text_to_num(openText)<<endl;
    string closedText = "";

    Magma_Encript(text_to_num(openText), num_to_text(closedText), key);
    system("pause");
    return 0;
}

