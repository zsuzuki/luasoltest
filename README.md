# SOL2(lua binder) Test

luaのバインダ[sol2](https://github.com/ThePhD/sol2)を使ったテストプログラムです。
libtoolingによるクラス情報の抽出をするプログラムも含みます。
libtoolingの使用方法については[こちら](https://qiita.com/Chironian/items/6021d35bf2750341d80c)を参考にしています。

## ビルド

```
$ gobuild && ninja
```

## 実行

```
$ ./build/default/Debug/soltest luafile
```
