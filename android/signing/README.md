# App signing key

Every OBDuke APK is signed with `obduke-release.jks` so new versions install over old ones
(and the in-app updater works). **Don't delete or replace it** — an APK signed with any other
key can't update an existing install; the app would have to be uninstalled first.

Keep a copy somewhere safe outside GitHub.

This is fine while the repo is private. Before making the repo public, move the key out:
add repo secrets `OBDUKE_KEYSTORE_B64` (`base64 -w0 obduke-release.jks`) and
`OBDUKE_KEYSTORE_PASSWORD`, then delete this folder. The build picks the secret up first.
