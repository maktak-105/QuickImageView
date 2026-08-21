# Tracker

Phase dashboard for the loop. Keep checkpoints small and verifiable. Use the
status legend so the doctor and other lanes can read progress mechanically.

## Status Legend

- `[ ]` not started
- `[~]` in progress
- `[x]` done and verified
- `[!]` blocked

## Checkpoints

- [~] 001 現状監査：既存実装・テスト・配布・Explorer登録の検証可能性を確定する（iter-2 review中）。
- [ ] 002 ファイル選択・単体起動の実機E2Eを赤化可能な形で整備する。
- [ ] 003 Explorer右クリック起動の実機E2Eを整備する。
- [ ] 004 ズーム・パン・右クリックメニュー・点滅を実機確認する。
- [ ] 005 変換保存・原本保護・配布物を検証する。

## Done When

- [ ] GoalのDone Whenをすべて満たし、各証拠JSONと独立レビューが存在する。

## Notes

- Record verification commands, evidence paths, and blockers next to each
  checkpoint as you close it.

- 1周目判定: 未完了。UI/Explorer証拠JSONの内容検証不足でFIX_REQUESTED。
- 次のアクション: iter-2の独立レビューで証拠内容検証とease-of-misuse解消を確認する。
