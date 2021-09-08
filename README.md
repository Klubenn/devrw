# devrw
Small program to read/write on block devices blocks of data

```
./devrw op=r|w dev=str bs=int count=int [sts=int] [p=int] [v=0|1] [sync=0|1]

op - операция чтения (r) или записи (w)
dev - путь до устройства
bs - размер блока для чтения/записи в кб
count - количество блоков для чтения/записи
sts - начальный сектор (по умолчанию 0)
p - пауза (в секундах) между операциями чтения/записи (по умолчанию 0)
v - вывод данных о текущей операции: 0 или 1 (по умолчанию 0)
sync - синхронизация при выполнении r/w операций (по умолчанию 1)
```

Compile:

`gcc devrw.c -o devrw`
