# ADR-0004 — ID and Equation Numbering

**Durum:** Accepted / V0.2.0 kod ve regression testleriyle uygulanmis durumda.

## Karar

```text
Node ID != Array Index != DOF ID != Equation ID
```

### Persistent entity ID

Node, element, field, set ve benzeri model varliklari kalici kimlik tasir. Storage yeniden siralansa bile kimlik degismez.

### Array position

Fortran array konumu implementasyon detayidir ve 1 tabanlidir. Model dosyasina veya element connectivity'ye kalici kimlik olarak yazilmaz.

### DOF ID

Bir fiziksel bilinmeyenin kimligidir. `(entity_id, field_id, component)` adresinden ayri saklanir. Node ID ile esitlenmez veya bit-packing/formul ile uretilmez.

### Equation ID

Yalnizca aktif solver bilinmeyenidir. Constraint'li DOF equation ID almaz. Equation ID 0 tabanli ve yeniden uretilebilir kabul edilir.

## Gerekce

Bu ayrim su gelecek ozellikleri desteklemek icin zorunludur:

- mesh reorder,
- sparse assembly,
- constrained/reduced systems,
- mixed `u-p`,
- rotational/thermal field ekleme,
- parallel partitioning,
- restart ve project persistence,
- solver backend degisimi.

## Test zorunlulugu

Her numbering refactor'u en az su regression'lari gecmelidir:

- ID degerleri array sirasindan bagimsiz olabilmeli,
- constrained DOF equation alamamali,
- mixed fields ayni equation map'te coexist edebilmeli,
- ayni input ayni deterministic numbering'i uretmeli.
