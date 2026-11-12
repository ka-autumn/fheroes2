# ブランチの運用手順

日本語化フォークのブランチ構成と、日々の手順を示す。
この構成に決定した理由は[ADR-0002](adr/0002-branch-structure.md)を参照する。

## ブランチ

```text
本家 ──► master          本家の鏡。開発者のコミットを置かない
            │
            ▼
         ja-engine       エンジン改造(日本語の表示、翻訳を読み込む仕組み)
            │
            ▼
         ja-localized    日本語版(ja-engine＋翻訳データ)。ビルドして遊ぶ対象
            │
            ▼
         ja-extended     日本語版＋独自要素(勇者、ユニット)。着手するときに作成する
```

変更は上から下へだけ流す。下のブランチの内容を上のブランチへ取り込まない。

## 変更をどのブランチに置くか

| 変更するもの | 置くブランチ |
|---|---|
| 日本語の表示や翻訳の読み込みに関わるC++のコード、ビルド設定、スクリプト | `ja-engine` |
| 上記の仕組みやブランチ運用の文書(`docs/`) | `ja-engine` |
| `files/lang/ja.po` | `ja-localized` |
| `files/lang/ja.map-*.tsv` | `ja-localized` |
| 勇者やユニットなど、ゲームの内容を変更するコードとデータ | `ja-extended` |

仕組みと翻訳データの両方が必要な作業は、先にコードを`ja-engine`へ入れて`ja-localized`へ流し、
その後に翻訳データを`ja-localized`へ入れる。

## 作業ブランチを切って戻す

作業は、変更を置くブランチから`work/<内容>`を切って行う。例は`ja-localized`でマップを翻訳する場合である。

```sh
git switch ja-localized
git switch -c work/map-winterlands
# 作業してコミットする
git switch ja-localized
git merge --no-ff work/map-winterlands
git branch -d work/map-winterlands
git push origin ja-localized
```

* fast-forwardできる場合でも`--no-ff`を付ける。1つの作業の範囲を履歴に残すためである。
* `work/*`はpushしない。
* `ja-engine`へ戻した場合は、続けて「下流へ流す」を行う。
* コードを変更した作業ブランチは、戻す前にビルドし、起動と日本語の表示を確認する。

## 下流へ流す

`ja-engine`が進んだら`ja-localized`へ取り込む。`ja-extended`を作成した後は、さらに`ja-extended`へ取り込む。

```sh
git switch ja-localized
git merge --no-ff ja-engine
git push origin ja-localized
```

## 本家の更新を取り込む

月に1回を目安に、作業の区切り(開いている作業ブランチを戻した後)で行う。

```sh
# 1. masterを本家に合わせる
git fetch upstream
git switch master
git merge --ff-only upstream/master
git push origin master

# 2. ja-engineへ取り込む。衝突はここで解消する
git switch ja-engine
git merge master
# ビルドして、起動と日本語の表示を確認する
git push origin ja-engine

# 3. 下流へ流す
git switch ja-localized
git merge --no-ff ja-engine
# ビルドして、起動と日本語の表示を確認する
git push origin ja-localized
```

* `upstream`は本家(`https://github.com/ihhub/fheroes2.git`)を指すリモートである。
* `master`で`--ff-only`が失敗した場合は、`master`に開発者のコミットが入っている。取り込みを止めて原因を調べる。
* 本家を`ja-localized`や`ja-extended`へ直接取り込まない。
* 取り込みにrebaseを使わない。
* 本家が文言を変更した場合は、取り込んだ後に`work/*`を切り、`ja.po`を新しい文言へ追従させる。

## 本家へPull Requestを提出する

`ja-engine`をそのまま提出しない。`master`から提出用のブランチを切り、`ja-engine`との差分から必要な部分を載せる。

```sh
git switch -c pr/<内容> master
git diff master ja-engine -- <提出するファイル> | git apply
# 本家の規約に合わせて整え、コミットする
```

## 退避用のブランチ

履歴を書き換える作業の前には、`backup/<元のブランチ名>-before-<作業>-<日付>`を作成する。
`backup/*`はpushしない。書き換えた結果を確認した後、開発者の判断で削除する。

## 現在の構成からの移行

2026-11-12時点の構成から、次の順で移行する。各手順は開発者の了承を得てから行う。

1. `feature/standard-map-translation-sidecar`の退避用のブランチを作成する。
2. `feature/cjk-support`を`ja-engine`へ、`japanese-localization`を`ja-localized`へ名前を変更する。
3. `feature/standard-map-translation-sidecar`の24コミットを、パスで2つに分け直す。
   * `files/lang/ja.map-*.tsv`以外(コード、ビルド設定、スクリプト、文書)は`ja-engine`から切った作業ブランチへ載せる。
   * `files/lang/ja.map-*.tsv`は翻訳データとして別に保持する。
   * 片方が空になるコミットは、その側では省く。
   * あわせて、ADR-0001の見出しをADR-0002と同じ日本語の見出しへ揃える。
4. コードと文書の作業ブランチを`ja-engine`へ`--no-ff`で戻し、`ja-engine`を`ja-localized`へ`--no-ff`で取り込む。
5. 翻訳データのコミットを`ja-localized`から切った作業ブランチへ載せ、`ja-localized`へ`--no-ff`で戻す。
6. `ja-localized`から`ja.po`を除いた内容が、元の`feature/standard-map-translation-sidecar`と一致することを
   `git diff`で確認する。
7. ビルドし、起動と日本語の表示、マップ翻訳の表示を確認する。
8. リモート`upstream`を追加し、「本家の更新を取り込む」を行う。
9. `ja-engine`と`ja-localized`を`origin`へpushする。
10. 別の作業ツリーが`feature/standard-map-translation-sidecar`を開いているため、`ja-localized`へ切り替える。
    確認が済んだ後、元のブランチと`backup/*`を削除する。
