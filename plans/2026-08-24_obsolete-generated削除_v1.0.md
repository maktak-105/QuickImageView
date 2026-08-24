# QuickImageView 旧生成物フォルダ削除 v1.0

作成日: 2026-08-24  
作業ディレクトリ: `C:\Users\makta\source\QuickImageView`

## 目的

ソース隣に残っていた生成物退避フォルダを削除し、ローカルの正本を `QuickImageView` 1本にする。

## 削除したもの

- `C:\Users\makta\source\QuickImageView-obsolete-generated-20260823`
- `C:\Users\makta\source\QuickImageView-obsolete-generated-20260823-dist-root`

いずれも `.git` なし。旧 CMake 中間物・MSI staging・exe/msi。正本 `QuickImageView` は未変更。

## 結果

`C:\Users\makta\source` 配下の `QuickImage*` は `QuickImageView` のみ。
