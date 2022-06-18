#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define GetArrayType_Size 4
#define FileNameArray_Size 500

enum ArrayType {
    Unknown,
    IsZip,
    IsRar
};

int GetFullPathToFile(char* PathToFile);
int CloseFile(FILE* mStreamPointer);
int AddToBuffer(int* Buffer, int* CurInd, int* MaxBufferInd, int NewChar);
int IsStringOccurrence(int* SampleArray, int* Buffer, int CurBufferInd, const int MaxBufferInd, const int MaxSampleArrayInd);
FILE* OpenFile(char* mPathToFile);
void FindArchiveStructure(FILE* StreamPointer);
void ShowIncludedFilesZip(FILE* StreamPointer);
void ShowFileName(int* FileNameBuffer, int* CurFileNameInd, const int ElementsNumber, int* FileCntr);
void KeepOpenWindow();
enum ArrayType GetArrayType(FILE* StreamPointer);

int main()
{
    char PathToFile[251];
    int CloseResult, PathLen = 0;
    FILE* StreamPointer;

    PathLen = GetFullPathToFile(PathToFile);
    if(PathLen == 0)
        return 0;

    StreamPointer = OpenFile(PathToFile);
    if(StreamPointer == NULL){
        printf("Error open file. Unknown file.");
        KeepOpenWindow();
        }
    else
        FindArchiveStructure(StreamPointer);

    if(StreamPointer != NULL){
        CloseResult = CloseFile(StreamPointer);
        if(CloseResult == EOF){
            printf("Error close file.");
            KeepOpenWindow();
        }
    }
    return 0;
}

int GetFullPathToFile(char* mPathToFile) {

    int PathLen = 0;

    printf("Enter full path to the file (max len - 250ch): ");
    scanf("%250s", mPathToFile);

    while(mPathToFile[PathLen] != '\0')
    {
        PathLen++;
    }

    return PathLen;

}

FILE* OpenFile(char* mPathToFile) {

    struct stat buff;
    FILE* mStreamPointer = NULL;

    // Проверяем, что введенный путь указывает на обычный файл. Если это файл - попытаемся открыть.
    if(stat(mPathToFile, &buff) == 0 && (buff.st_mode & __S_IFMT) == __S_IFREG)
        mStreamPointer = fopen(mPathToFile, "rb");

    return mStreamPointer;

}

int CloseFile(FILE* mStreamPointer) {

    return fclose(mStreamPointer);

}

void FindArchiveStructure(FILE* StreamPointer){

    enum ArrayType AType;

    AType = GetArrayType(StreamPointer);

    if(AType == IsZip)
        ShowIncludedFilesZip(StreamPointer);
    else{
        // Держем окно терминала открытым.
        printf("\nArchive not found.");
        KeepOpenWindow();
    }

}

// Определяет тип присоединенного архива.
enum ArrayType GetArrayType(FILE* StreamPointer){

    enum ArrayType Result = Unknown;

    int BeginSignatureZipArray[] = {0x04, 0x03, 0x4B, 0x50}; // Сигнатура в обратном порядке
    const int MaxInd = 3;

    int IsZipArray = 0;

    int Buffer[GetArrayType_Size] = {0};
    int CurBufferInd = 0;
    int MaxBufferInd = GetArrayType_Size - 1;

    while(AddToBuffer(Buffer, &CurBufferInd, &MaxBufferInd, fgetc(StreamPointer)) != EOF){

        IsZipArray = IsStringOccurrence(BeginSignatureZipArray, Buffer, CurBufferInd, MaxBufferInd, MaxInd);
        if(IsZipArray == 1) {
            Result = IsZip;
            break;
        }
    }

    return Result;

}

// Ищем и выводм в консоль файлы в zip архиве.
void ShowIncludedFilesZip(FILE* StreamPointer){

    // Сигнатуры указаны в обратном порядке.
    int BeginFileMetaArray[] = {0x02, 0x01, 0x4b, 0x50};
    int EndFileMetaArray[] = {0x00, 0x05, 0x54, 0x55};
    int EndZipArray[] = {0x06, 0x05, 0x4b, 0x50};
    const int MaxInd = 3;
    const int ElementsNumber = 4;

    int IsBeginFileMeta = 0;
    int IsEndFileMeta = 0;
    int IsFillNameArray = 0;

    int FileNameBuffer[FileNameArray_Size];
    int CurFileNameInd = 0;
    int MaxFileNameInd = FileNameArray_Size - 1;

    int Buffer[GetArrayType_Size] = {0};
    int CurBufferInd = 0;
    int MaxBufferInd = GetArrayType_Size - 1;

    int NewChar;
    int FileCntr = 0;
    long int offset = 42; // Смещение на 42 байта

    printf("\nFile list:\n");

    while((NewChar = AddToBuffer(Buffer, &CurBufferInd, &MaxBufferInd, fgetc(StreamPointer))) != EOF){

        // Начало секции с именем файла.
        IsBeginFileMeta = IsStringOccurrence(BeginFileMetaArray, Buffer, CurBufferInd, MaxBufferInd, MaxInd);
        if(IsBeginFileMeta == 1) {
            // Cмещаемся на 42 байта к началу имени файла/
            fseek(StreamPointer, offset, SEEK_CUR);
            // Выставляем признак начала заполнения массива имени файла
            IsFillNameArray = 1;
            continue;
        }

        //Если зашли в секцию с имененм файла - сохраняем байты в буфер.
        if(IsFillNameArray == 1 && CurFileNameInd < MaxFileNameInd){
            FileNameBuffer[CurFileNameInd] = NewChar;
            CurFileNameInd++;
        }

        // Конец секции с имененм файла
        IsEndFileMeta = IsStringOccurrence(EndFileMetaArray, Buffer, CurBufferInd, MaxBufferInd, MaxInd);
        if(IsEndFileMeta == 1) {
            // Выводим имя файла.
            ShowFileName(FileNameBuffer, &CurFileNameInd, ElementsNumber, &FileCntr);
            // Перестаем заполнять массив имени файла
            IsFillNameArray = 0;
        }

        // Конец zip архива
        if(IsStringOccurrence(EndZipArray, Buffer, CurBufferInd, MaxBufferInd, MaxInd) == 1) {
            ShowFileName(FileNameBuffer, &CurFileNameInd, ElementsNumber, &FileCntr);
            break;
        }
    }

    // Держем окно терминала открытым.
    printf("\nFile output completed.");
    printf("\nTotal: ");
    printf("%d", FileCntr);
    KeepOpenWindow();

}

void ShowFileName(int* FileNameBuffer, int* CurFileNameInd, const int ElementsNumber, int* FileCntr){

    int i;
    char locFileNameBuffer[FileNameArray_Size+1];

    if((*CurFileNameInd) == 0)
        return;

    // Убираем сигнатуры
    *CurFileNameInd = (*CurFileNameInd) - ElementsNumber;

    // Если последний символ - "/" или "\", то эта папка, не выводим.
    if(*CurFileNameInd > 0  &&
       !(FileNameBuffer[(*CurFileNameInd)-1] == '/' || FileNameBuffer[(*CurFileNameInd)-1] == '\\')){

        FileNameBuffer[*CurFileNameInd] = '\0';
        for(i = 0; FileNameBuffer[i] != '\0'; i++){
            locFileNameBuffer[i] = (char) FileNameBuffer[i];
        }
        locFileNameBuffer[i++] =  '\n';
        locFileNameBuffer[i++] =  '\0';

        printf("%s", locFileNameBuffer);
        (*FileCntr)++;
    }
    *CurFileNameInd = 0;
}

// Добавляем новый символ во временный буфер.
// Если дошли до конца буфера - начинаем с начала.
int AddToBuffer(int* Buffer, int* CurBufferInd, int* MaxBufferInd, int NewChar){

    // Если вышли за приделы массива - сбразываем индекс на ноль.
    if(*CurBufferInd > *MaxBufferInd)
        *CurBufferInd = 0;

    Buffer[*CurBufferInd] = NewChar;

    (*CurBufferInd)++;

    return NewChar;

}

// Метод определяет вхождение строки в строку.
int IsStringOccurrence(int* SampleArray, int* Buffer, int CurBufferInd, const int MaxBufferInd, const int MaxSampleArrayInd){

    int Result = 1;

    CurBufferInd--;
    if(CurBufferInd < 0)
        CurBufferInd = MaxBufferInd;

    for(int i = 0; i <= MaxSampleArrayInd; i++){
        if(SampleArray[i] != Buffer[CurBufferInd]){
            Result = 0;
            break;
        }

        CurBufferInd--;
        if(CurBufferInd < 0)
            CurBufferInd = MaxBufferInd;
    }

    return Result;

}

// Просто держит окно терминала открытым.
void KeepOpenWindow(){

    char EmptyEnter[1];
    scanf("%1s", EmptyEnter);
}
