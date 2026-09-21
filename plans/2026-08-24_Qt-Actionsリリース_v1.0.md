# Qtブランチ GitHub Actions リリース v1.0

作成日: 2026-08-24  
作業ディレクトリ: `%USERPROFILE%\source\QuickImageView`  
ブランチ: `qt`  
タグ: `v3.1.2-qt`

## 目的

`main` に混ぜず、`qt` ブランチ先端から GitHub Actions で Qt 版 ZIP を Release する。

## 実施

- `build-tools/deploy_qt.py` を切り出し、ローカル MinGW と Actions MSVC の両方で使う。
- `ci.yml` を `qt` ブランチでも実行。
- `release.yml` を Qt 6.10.3 MSVC ビルド → テスト → windeployqt → `package.ps1 -Qt` に変更。
- 成果物: `QuickImageViewQt-v3.1.2-win64.zip`

## 検証

タグ push 後の Actions が成功し、Release に ZIP が付くこと。
