# Security Policy

## Supported line

V1.0.x kaynak serisi güvenlik ve corrupted-input hardening için desteklenen ilk release çizgisidir.

## Raporlama

Güvenlik açığını public issue içinde exploit ayrıntısıyla yayınlamak yerine proje sahibine özel kanaldan bildirin. Public repository üzerinde özel security advisory özelliği etkinse öncelikle onu kullanın.

## Güvenilmeyen dosyalar

FEMCAE project JSON, STEP, DXF, Abaqus INP ve checkpoint dosyaları güvenilmeyen input olarak ele alınmalıdır. Bir parser'ın malformed input'ta crash, out-of-bounds, sonsuz loop veya partial-state corruption üretmesi security/reliability bug kabul edilir.

## Binary dağıtım

Resmi macOS binary yalnız release checklist'teki architecture, dependency, codesign ve notarization gate'leri tamamlandıktan sonra güvenilir dağıtım artifact'ı olarak etiketlenmelidir.
