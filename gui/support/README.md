# GUI Support and Acceptance

`gui/support` production model state sahibi değildir. Bu dizin, gerçek Dynamics26 uygulama composition'ı üzerinde çalışan native self-test, screenshot/audit ve acceptance yardımcılarını içerir.

## Alpha.3.6 Named Selection sözleşmesi

- Named Selection bir `ProjectObject` + persistent `ScopeReference` mühendislik nesnesidir; raw viewport seçimi değildir.
- Normal görünümde viewport domain ve entity kind, `ObjectType` tahmininden değil kayıtlı scope'tan çözülür.
- Geçerli persistent scope yalnız doğrulanmış CAD/FEM provenance üzerinden overlay olarak gösterilir; `SelectionManager` içine transient seçim gibi kopyalanmaz.
- Stale veya dangling scope eski `GeometryEntityId` / `MeshEntityId` değerlerini preload veya highlight etmez.
- `Edit Selection` geçerli kayıtlı scope'u transient düzenleme başlangıcı olarak yükleyebilen tek akıştır. Stale scope edit moduna doğru domain/kind ile fakat boş transient seçimle girer.
- `Apply Selection` kalıcı scope değişimini tek `ReplaceNamedSelectionScopeCommand` Undo adımı olarak yazar.
- `Cancel` yalnız view/transient state'i kapatır; document mutation veya Undo girdisi oluşturmaz.
- Fixed Support / Force consumer'ları Named Selection entity ID'lerini kopyalamaz; persistent Named Selection `ObjectId` referansını çözer.

## Doğrulama dili

- `CODE EXISTS`: kaynakta uygulama vardır.
- `TEST EXISTS`: acceptance/regression kontrolü kaynakta vardır.
- `TEST PASSED`: ilgili native CI/self-test gerçekten başarıyla çalışmıştır.
- `FEATURE WORKS`: gerekli otomatik acceptance kriterleri birlikte karşılanmıştır.
- `USER VALIDATED`: fiziksel mouse/trackpad ve gerçek kullanıcı UX kabulü ayrıca yapılmıştır.

Otomatik testler fiziksel kullanıcı kabulünün yerine geçmez.