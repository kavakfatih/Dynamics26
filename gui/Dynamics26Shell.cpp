#include "Dynamics26Shell.h"

#include <femcae/femcae.h>

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#ifndef DYNAMICS26_GUI_MILESTONE
#define DYNAMICS26_GUI_MILESTONE "1.1.0-alpha.1"
#endif

namespace {

constexpr int kNavigatorRoleInspectorPage = Qt::UserRole + 1;
constexpr int kNavigatorOpenResults = -100;

struct WorkspaceParts {
    QTreeWidget *navigator = nullptr;
    QWidget *viewport = nullptr;
    QWidget *legacyInspector = nullptr;
};

struct InspectorShell {
    QFrame *panel = nullptr;
    QStackedWidget *pages = nullptr;
    QLabel *context = nullptr;
    QLabel *emptyState = nullptr;
};

QWidget *makeExpandingSpacer(QWidget *parent)
{
    auto *spacer = new QWidget(parent);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return spacer;
}

QLabel *makePanelTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.0));
    label->setFont(font);
    return label;
}

void setStandardIcon(QMainWindow &window, QAction *action, QStyle::StandardPixmap pixmap)
{
    if (action != nullptr && window.style() != nullptr) {
        action->setIcon(window.style()->standardIcon(pixmap));
    }
}

bool invokeMainWindowSlot(QMainWindow &window, const char *slotName)
{
    return QMetaObject::invokeMethod(&window, slotName, Qt::DirectConnection);
}

QAction *makeSlotAction(
    QMainWindow &window,
    const QString &text,
    const QString &toolTip,
    const char *slotName,
    QStyle::StandardPixmap icon)
{
    auto *action = new QAction(text, &window);
    action->setToolTip(toolTip);
    setStandardIcon(window, action, icon);
    const QByteArray target(slotName);
    QObject::connect(action, &QAction::triggered, &window, [&window, target] {
        if (!invokeMainWindowSlot(window, target.constData())) {
            QMessageBox::warning(
                &window,
                QStringLiteral("Dynamics26"),
                QStringLiteral("Komut mevcut mühendislik katmanına bağlanamadı: %1")
                    .arg(QString::fromLatin1(target)));
        }
    });
    return action;
}

WorkspaceParts detachLegacyWorkspace(QMainWindow &window)
{
    WorkspaceParts parts;
    auto *legacySplitter = qobject_cast<QSplitter *>(window.centralWidget());
    if (legacySplitter == nullptr || legacySplitter->count() < 3) {
        return parts;
    }

    parts.navigator = qobject_cast<QTreeWidget *>(legacySplitter->widget(0));
    parts.viewport = legacySplitter->widget(1);
    parts.legacyInspector = legacySplitter->widget(2);
    if (parts.navigator == nullptr || parts.viewport == nullptr || parts.legacyInspector == nullptr) {
        return WorkspaceParts {};
    }

    // Corrective Alpha.1'in temel farkı: görünür workspace artık eski
    // MainWindow splitter'ını yalnız yeniden etiketlemez. Çalışan widget'lar
    // servis/işlevleri korunarak eski taşıyıcıdan ayrılır ve shell-owned yeni
    // layout'a yerleştirilir.
    parts.navigator->setParent(nullptr);
    parts.viewport->setParent(nullptr);
    parts.legacyInspector->setParent(nullptr);
    legacySplitter->deleteLater();
    return parts;
}

void configureNavigatorWidget(QTreeWidget *navigator)
{
    if (navigator == nullptr) {
        return;
    }

    navigator->setObjectName(QStringLiteral("Dynamics26Navigator"));
    navigator->setHeaderHidden(true);
    navigator->setRootIsDecorated(true);
    navigator->setUniformRowHeights(true);
    navigator->setIndentation(14);
    navigator->setFrameShape(QFrame::NoFrame);
    navigator->setSelectionMode(QAbstractItemView::SingleSelection);
    navigator->setAccessibleName(QStringLiteral("Proje Gezgini"));

    // Alpha.2'de gerçek project object modeli gelecektir. Corrective Alpha.1
    // ise debug/solver implementation ağacını göstermemeli; yalnız gerçekten
    // bağlı mühendislik yüzeylerine giden sığ ve anlaşılır bölümler tutulur.
    navigator->clear();

    auto addSection = [navigator](const QString &name, int inspectorPage) {
        auto *item = new QTreeWidgetItem(navigator, {name});
        item->setData(0, kNavigatorRoleInspectorPage, inspectorPage);
        return item;
    };

    addSection(QStringLiteral("Geometri"), 0);
    addSection(QStringLiteral("Malzemeler"), 2);
    addSection(QStringLiteral("Kesitler"), 3);
    addSection(QStringLiteral("Mesh"), 1);

    auto *analyses = addSection(QStringLiteral("Analizler"), 5);
    auto *loads = new QTreeWidgetItem(analyses, {QStringLiteral("Yükler ve Sınır Şartları")});
    loads->setData(0, kNavigatorRoleInspectorPage, 4);
    auto *settings = new QTreeWidgetItem(analyses, {QStringLiteral("Analiz Ayarları")});
    settings->setData(0, kNavigatorRoleInspectorPage, 5);
    analyses->setExpanded(true);

    // MainWindow'ın mevcut result-tree güncelleme sözleşmesi top-level
    // "Sonuçlar" öğesini arar. Bu isim çalışan yolu kırmamak için korunur;
    // sayısal değerler ise aşağıda result table'dan nesne adı olarak normalize edilir.
    addSection(QStringLiteral("Sonuçlar"), kNavigatorOpenResults);
    navigator->clearSelection();
    navigator->setCurrentItem(nullptr);
}

QWidget *wrapNavigator(QTreeWidget *navigator)
{
    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("Dynamics26NavigatorPanel"));
    panel->setFrameShape(QFrame::NoFrame);
    panel->setMinimumWidth(210);
    panel->setMaximumWidth(350);
    panel->setAutoFillBackground(true);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 8, 8);
    layout->setSpacing(6);
    layout->addWidget(makePanelTitle(QStringLiteral("PROJE"), panel));
    layout->addWidget(navigator, 1);
    return panel;
}

void setLayoutItemVisible(QLayoutItem *item, bool visible)
{
    if (item == nullptr) {
        return;
    }
    if (auto *widget = item->widget()) {
        widget->setVisible(visible);
    }
    if (auto *layout = item->layout()) {
        for (int i = 0; i < layout->count(); ++i) {
            setLayoutItemVisible(layout->itemAt(i), visible);
        }
    }
}

QString formRowLabel(const QFormLayout *form, int row)
{
    if (form == nullptr || row < 0 || row >= form->rowCount()) {
        return QString();
    }
    for (const auto role : {QFormLayout::LabelRole, QFormLayout::SpanningRole}) {
        if (auto *item = form->itemAt(row, role)) {
            if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                return label->text();
            }
        }
    }
    return QString();
}

void setFormRowVisible(QFormLayout *form, int row, bool visible)
{
    if (form == nullptr || row < 0 || row >= form->rowCount()) {
        return;
    }
    for (const auto role : {QFormLayout::LabelRole, QFormLayout::FieldRole, QFormLayout::SpanningRole}) {
        setLayoutItemVisible(form->itemAt(row, role), visible);
    }
}

int findFormRow(QFormLayout *form, const QString &text)
{
    if (form == nullptr) {
        return -1;
    }
    for (int row = 0; row < form->rowCount(); ++row) {
        if (formRowLabel(form, row).contains(text, Qt::CaseInsensitive)) {
            return row;
        }
    }
    return -1;
}

void setFormRowVisible(QFormLayout *form, const QString &text, bool visible)
{
    const int row = findFormRow(form, text);
    if (row >= 0) {
        setFormRowVisible(form, row, visible);
    }
}

QWidget *formFieldWidget(QFormLayout *form, const QString &text)
{
    const int row = findFormRow(form, text);
    if (row < 0) {
        return nullptr;
    }
    if (auto *item = form->itemAt(row, QFormLayout::FieldRole)) {
        return item->widget();
    }
    return nullptr;
}

void renameFormLabel(QFormLayout *form, const QString &from, const QString &to)
{
    const int row = findFormRow(form, from);
    if (row < 0) {
        return;
    }
    if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
        if (auto *label = qobject_cast<QLabel *>(item->widget())) {
            label->setText(to);
            label->setWordWrap(true);
            return;
        }
    }
    if (auto *item = form->itemAt(row, QFormLayout::SpanningRole)) {
        if (auto *label = qobject_cast<QLabel *>(item->widget())) {
            label->setText(to);
            label->setWordWrap(true);
        }
    }
}

void appendRowWidgets(QFormLayout *form, int row, QSet<QWidget *> &widgets)
{
    if (form == nullptr || row < 0 || row >= form->rowCount()) {
        return;
    }
    for (const auto role : {QFormLayout::LabelRole, QFormLayout::FieldRole, QFormLayout::SpanningRole}) {
        if (auto *item = form->itemAt(row, role)) {
            if (auto *widget = item->widget()) {
                widgets.insert(widget);
            }
        }
    }
}

void polishCommonInspectorPage(QWidget *page)
{
    if (page == nullptr) {
        return;
    }

    page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    if (page->layout() != nullptr) {
        page->layout()->setContentsMargins(4, 4, 8, 12);
        page->layout()->setSpacing(8);
    }

    const auto forms = page->findChildren<QFormLayout *>();
    for (auto *form : forms) {
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        form->setFormAlignment(Qt::AlignTop);
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        form->setHorizontalSpacing(10);
        form->setVerticalSpacing(8);
    }

    const auto labels = page->findChildren<QLabel *>();
    for (auto *label : labels) {
        if (label->text().size() > 36 || label->text().contains(QChar(0x2192))) {
            label->setWordWrap(true);
        }
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    const auto buttons = page->findChildren<QPushButton *>();
    for (auto *button : buttons) {
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    const auto combos = page->findChildren<QComboBox *>();
    for (auto *combo : combos) {
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        combo->setMinimumContentsLength(8);
    }
    const auto doubles = page->findChildren<QDoubleSpinBox *>();
    for (auto *spin : doubles) {
        spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    const auto spins = page->findChildren<QSpinBox *>();
    for (auto *spin : spins) {
        spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
}

void polishMaterialPage(QWidget *page)
{
    auto *form = page != nullptr ? page->findChild<QFormLayout *>() : nullptr;
    if (form == nullptr) {
        return;
    }

    renameFormLabel(form, QStringLiteral("Bulk Modulus K"), QStringLiteral("Hacim Modülü K"));

    QComboBox *model = nullptr;
    const auto combos = page->findChildren<QComboBox *>();
    for (auto *combo : combos) {
        if (combo->findText(QStringLiteral("Neo-Hookean")) >= 0 && combo->findText(QStringLiteral("Ogden")) >= 0) {
            model = combo;
            break;
        }
    }
    if (model == nullptr) {
        return;
    }

    QPushButton *preview = nullptr;
    const auto buttons = page->findChildren<QPushButton *>();
    for (auto *button : buttons) {
        if (button->text().contains(QStringLiteral("Malzeme Eğrisini"), Qt::CaseInsensitive)) {
            preview = button;
            break;
        }
    }

    auto *termCount = qobject_cast<QSpinBox *>(formFieldWidget(form, QStringLiteral("Ogden Terim Sayısı")));
    const auto updateRows = [form, preview, model, termCount] {
        const int index = model->currentIndex();
        const int terms = termCount != nullptr ? termCount->value() : 3;

        setFormRowVisible(form, QStringLiteral("Young Modülü"), index == 0);
        setFormRowVisible(form, QStringLiteral("Poisson"), index == 0);
        setFormRowVisible(form, QStringLiteral("Hacim Modülü"), index != 0);
        setFormRowVisible(form, QStringLiteral("C10"), index >= 1 && index <= 3);
        setFormRowVisible(form, QStringLiteral("C01"), index == 2);
        setFormRowVisible(form, QStringLiteral("C20"), index == 3);
        setFormRowVisible(form, QStringLiteral("C30"), index == 3);
        setFormRowVisible(form, QStringLiteral("Ogden Terim Sayısı"), index == 4);
        for (int i = 1; i <= 3; ++i) {
            setFormRowVisible(form, QStringLiteral("Ogden μ%1").arg(i), index == 4 && i <= terms);
            setFormRowVisible(form, QStringLiteral("Ogden α%1").arg(i), index == 4 && i <= terms);
        }
        if (preview != nullptr) {
            preview->setVisible(index != 0);
        }
    };

    QObject::connect(model, qOverload<int>(&QComboBox::currentIndexChanged), page, [updateRows](int) {
        updateRows();
    });
    if (termCount != nullptr) {
        QObject::connect(termCount, qOverload<int>(&QSpinBox::valueChanged), page, [updateRows](int) {
            updateRows();
        });
    }
    updateRows();
}

void polishSectionPage(QWidget *page)
{
    auto *form = page != nullptr ? page->findChild<QFormLayout *>() : nullptr;
    if (form != nullptr) {
        renameFormLabel(form, QStringLiteral("Demo Uzunluğu"), QStringLiteral("Uzunluk"));
    }
}

void polishLoadPage(QWidget *page)
{
    if (page == nullptr) {
        return;
    }
    const auto buttons = page->findChildren<QPushButton *>();
    for (auto *button : buttons) {
        if (button->text().contains(QStringLiteral("Demo"), Qt::CaseInsensitive)) {
            button->hide();
        }
    }
    const auto labels = page->findChildren<QLabel *>();
    for (auto *label : labels) {
        if (label->text().contains(QStringLiteral("Henüz çözüm yok"), Qt::CaseInsensitive)) {
            label->hide();
        }
    }
}

void polishAnalysisPage(QWidget *page)
{
    auto *form = page != nullptr ? page->findChild<QFormLayout *>() : nullptr;
    if (form == nullptr) {
        return;
    }

    renameFormLabel(form, QStringLiteral("Formulation"), QStringLiteral("Formülasyon"));
    renameFormLabel(form, QStringLiteral("Mixed Shear γ"), QStringLiteral("Karışık Formülasyon Kayması γ"));
    renameFormLabel(form, QStringLiteral("Contact Enforcement"), QStringLiteral("Temas Uygulama Yöntemi"));
    renameFormLabel(form, QStringLiteral("Contact k_n / E"), QStringLiteral("Temas Penalty Oranı kₙ/E"));
    renameFormLabel(form, QStringLiteral("İlk Load Increment"), QStringLiteral("İlk Yük Artımı"));
    renameFormLabel(form, QStringLiteral("Minimum Increment"), QStringLiteral("Minimum Artım"));
    renameFormLabel(form, QStringLiteral("Maximum Increment"), QStringLiteral("Maksimum Artım"));
    renameFormLabel(form, QStringLiteral("Max Iteration"), QStringLiteral("Maksimum İterasyon"));
    renameFormLabel(form, QStringLiteral("Nonlinear Static / Large Displacement"), QStringLiteral("Nonlineer Statik / Büyük Deformasyon"));

    const auto buttons = page->findChildren<QPushButton *>();
    for (auto *button : buttons) {
        if (button->text().contains(QStringLiteral("Demo"), Qt::CaseInsensitive)) {
            button->hide();
        }
    }

    const auto labels = page->findChildren<QLabel *>();
    for (auto *label : labels) {
        if (label->text().contains(QStringLiteral("solver çekirdeği"), Qt::CaseInsensitive)
            || label->text().contains(QStringLiteral("Henüz modal çözüm yok"), Qt::CaseInsensitive)
            || label->text().contains(QStringLiteral("Henüz nonlinear çözüm yok"), Qt::CaseInsensitive)) {
            label->hide();
        }
    }

    const auto checks = page->findChildren<QCheckBox *>();
    for (auto *check : checks) {
        if (check->text().contains(QStringLiteral("Backtracking line search"), Qt::CaseInsensitive)) {
            check->setText(QStringLiteral("Line Search"));
        } else if (check->text().contains(QStringLiteral("Adaptive increment"), Qt::CaseInsensitive)) {
            check->setText(QStringLiteral("Uyarlamalı Artım / Cutback"));
        }
    }

    const QStringList advancedRows = {
        QStringLiteral("Formülasyon"),
        QStringLiteral("Karışık Formülasyon Kayması"),
        QStringLiteral("Temas Uygulama Yöntemi"),
        QStringLiteral("Temas Penalty Oranı"),
        QStringLiteral("Newton Yöntemi"),
        QStringLiteral("İlk Yük Artımı"),
        QStringLiteral("Minimum Artım"),
        QStringLiteral("Maksimum Artım"),
        QStringLiteral("Maksimum İterasyon")
    };

    QSet<QWidget *> advancedWidgets;
    int firstAdvancedRow = form->rowCount();
    for (const auto &name : advancedRows) {
        const int row = findFormRow(form, name);
        if (row >= 0) {
            firstAdvancedRow = qMin(firstAdvancedRow, row);
            appendRowWidgets(form, row, advancedWidgets);
        }
    }
    for (auto *check : checks) {
        advancedWidgets.insert(check);
    }

    auto *advancedButton = new QToolButton(page);
    advancedButton->setObjectName(QStringLiteral("Dynamics26AdvancedSolverDisclosure"));
    advancedButton->setText(QStringLiteral("Gelişmiş Çözücü Ayarları"));
    advancedButton->setCheckable(true);
    advancedButton->setChecked(false);
    advancedButton->setArrowType(Qt::RightArrow);
    advancedButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    advancedButton->setAutoRaise(true);
    advancedButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    if (firstAdvancedRow < form->rowCount()) {
        form->insertRow(firstAdvancedRow, advancedButton);
    } else {
        form->addRow(advancedButton);
    }

    const auto setAdvancedVisible = [advancedWidgets, advancedButton](bool visible) {
        advancedButton->setArrowType(visible ? Qt::DownArrow : Qt::RightArrow);
        for (auto *widget : advancedWidgets) {
            if (widget != nullptr) {
                widget->setVisible(visible);
            }
        }
    };
    QObject::connect(advancedButton, &QToolButton::toggled, page, setAdvancedVisible);
    setAdvancedVisible(false);
}

QWidget *makeScrollableInspectorPage(QWidget *page, QStackedWidget *parent, int pageIndex)
{
    if (page == nullptr || parent == nullptr) {
        return page;
    }

    polishCommonInspectorPage(page);
    if (pageIndex == 2) {
        polishMaterialPage(page);
    } else if (pageIndex == 3) {
        polishSectionPage(page);
    } else if (pageIndex == 4) {
        polishLoadPage(page);
    } else if (pageIndex == 5) {
        polishAnalysisPage(page);
    }

    auto *scroll = new QScrollArea(parent);
    scroll->setObjectName(QStringLiteral("Dynamics26InspectorScroll%1").arg(pageIndex));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scroll->setWidget(page);
    return scroll;
}

InspectorShell wrapInspector(QWidget *legacyInspector)
{
    InspectorShell shell;
    if (legacyInspector == nullptr) {
        return shell;
    }

    shell.panel = new QFrame;
    shell.panel->setObjectName(QStringLiteral("Dynamics26InspectorPanel"));
    shell.panel->setFrameShape(QFrame::NoFrame);
    shell.panel->setMinimumWidth(320);
    shell.panel->setMaximumWidth(460);
    shell.panel->setAutoFillBackground(true);
    shell.panel->setAccessibleName(QStringLiteral("Özellikler"));

    auto *layout = new QVBoxLayout(shell.panel);
    layout->setContentsMargins(10, 10, 10, 8);
    layout->setSpacing(6);
    layout->addWidget(makePanelTitle(QStringLiteral("ÖZELLİKLER"), shell.panel));

    shell.context = new QLabel(QStringLiteral("Seçim yok"), shell.panel);
    QFont contextFont = shell.context->font();
    contextFont.setBold(true);
    shell.context->setFont(contextFont);
    shell.context->setWordWrap(true);
    layout->addWidget(shell.context);

    shell.emptyState = new QLabel(
        QStringLiteral("Proje Gezgini’nden bir öğe seçerek mühendislik özelliklerini görüntüleyin."),
        shell.panel);
    shell.emptyState->setWordWrap(true);
    shell.emptyState->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    shell.emptyState->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(shell.emptyState, 1);

    shell.pages = new QStackedWidget(shell.panel);
    shell.pages->setObjectName(QStringLiteral("Dynamics26EngineeringInspector"));
    shell.pages->setAccessibleName(QStringLiteral("Mühendislik Özellikleri"));

    // MainWindow halen Alpha.1 öncesi engineering sayfalarını QTabWidget içinde
    // üretir. Corrective shell çalışan sayfaları kendi responsive stacked/scroll
    // container'ına taşır. Widget pointer'ları aynı kaldığı için geometry, mesh,
    // material ve solver bağlantıları korunurken eski tab chrome'u görünür olmaz.
    if (auto *legacyTabs = qobject_cast<QTabWidget *>(legacyInspector)) {
        int pageIndex = 0;
        while (legacyTabs->count() > 0) {
            QWidget *page = legacyTabs->widget(0);
            legacyTabs->removeTab(0);
            if (page != nullptr) {
                shell.pages->addWidget(makeScrollableInspectorPage(page, shell.pages, pageIndex));
            }
            ++pageIndex;
        }
        legacyTabs->setParent(nullptr);
        legacyTabs->deleteLater();
    } else {
        legacyInspector->setParent(nullptr);
        shell.pages->addWidget(makeScrollableInspectorPage(legacyInspector, shell.pages, 0));
    }

    shell.pages->hide();
    layout->addWidget(shell.pages, 1);
    return shell;
}

int tabIndexContaining(QTabWidget *tabs, const QString &text)
{
    if (tabs == nullptr) {
        return -1;
    }
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i).contains(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

void activateUtilityTab(QDockWidget *dock, QTabWidget *tabs, const QString &text)
{
    if (dock == nullptr || tabs == nullptr) {
        return;
    }
    dock->show();
    const int index = tabIndexContaining(tabs, text);
    if (index >= 0) {
        tabs->setCurrentIndex(index);
    }
}

void configureUtilityArea(QDockWidget *dock)
{
    if (dock == nullptr) {
        return;
    }

    dock->setObjectName(QStringLiteral("Dynamics26UtilityArea"));
    dock->setWindowTitle(QStringLiteral("Sonuçlar ve Tanılama"));
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetClosable);
    dock->setMinimumHeight(110);

    auto *tabs = dock->findChild<QTabWidget *>();
    if (tabs == nullptr) {
        return;
    }

    tabs->setObjectName(QStringLiteral("Dynamics26UtilityTabs"));
    tabs->setDocumentMode(false);
    for (int i = 0; i < tabs->count(); ++i) {
        const QString title = tabs->tabText(i);
        if (title.contains(QStringLiteral("Sonuç"), Qt::CaseInsensitive)
            || title.contains(QStringLiteral("Result"), Qt::CaseInsensitive)) {
            tabs->setTabText(i, QStringLiteral("Sonuçlar"));
        } else if (title.contains(QStringLiteral("Yakınsama"), Qt::CaseInsensitive)
                   || title.contains(QStringLiteral("Convergence"), Qt::CaseInsensitive)) {
            tabs->setTabText(i, QStringLiteral("Yakınsama"));
        } else if (title.contains(QStringLiteral("Log"), Qt::CaseInsensitive)) {
            tabs->setTabText(i, QStringLiteral("Günlük"));
        }
    }
}

QString normalizedResultObjectName(const QString &name)
{
    if (name.contains(QStringLiteral("Mesh max |u|"), Qt::CaseInsensitive)
        || name.contains(QStringLiteral("Tip Deplasman"), Qt::CaseInsensitive)) {
        return QStringLiteral("Toplam Deformasyon");
    }
    if (name.contains(QStringLiteral("von Mises"), Qt::CaseInsensitive)) {
        return QStringLiteral("von Mises Gerilmesi");
    }
    if (name.contains(QStringLiteral("Axial Stress"), Qt::CaseInsensitive)) {
        return QStringLiteral("Eksenel Gerilme");
    }
    if (name.contains(QStringLiteral("Mesnet Reaksiyonu"), Qt::CaseInsensitive)
        || name.contains(QStringLiteral("ΣRx"), Qt::CaseInsensitive)) {
        return QStringLiteral("Reaksiyon Kuvveti");
    }
    if (name.contains(QStringLiteral("Probe Node"), Qt::CaseInsensitive)) {
        return QStringLiteral("Probe Düğümü");
    }
    if (name.contains(QStringLiteral("Probe ux"), Qt::CaseInsensitive)) {
        return QStringLiteral("Probe Deplasmanı");
    }
    if (name.contains(QStringLiteral("Mode 1"), Qt::CaseInsensitive)) {
        return QStringLiteral("Mod 1 Frekansı");
    }
    if (name.contains(QStringLiteral("Mode 2"), Qt::CaseInsensitive)) {
        return QStringLiteral("Mod 2 Frekansı");
    }
    if (name.contains(QStringLiteral("Completed Load Factor"), Qt::CaseInsensitive)) {
        return QStringLiteral("Tamamlanan Yük Faktörü");
    }
    if (name.contains(QStringLiteral("Accepted Steps"), Qt::CaseInsensitive)) {
        return QStringLiteral("Kabul Edilen Adımlar");
    }
    if (name.contains(QStringLiteral("Newton Corrections"), Qt::CaseInsensitive)) {
        return QStringLiteral("Newton Düzeltmeleri");
    }
    if (name.contains(QStringLiteral("Cutbacks"), Qt::CaseInsensitive)) {
        return QStringLiteral("Cutback Sayısı");
    }
    if (name.contains(QStringLiteral("Final Residual Norm"), Qt::CaseInsensitive)) {
        return QStringLiteral("Son Residual Normu");
    }
    return name;
}

void synchronizeResultNavigator(QTreeWidget *navigator, QTabWidget *tabs)
{
    if (navigator == nullptr || tabs == nullptr) {
        return;
    }

    const int resultTab = tabIndexContaining(tabs, QStringLiteral("Sonuç"));
    auto *table = resultTab >= 0 ? qobject_cast<QTableWidget *>(tabs->widget(resultTab)) : nullptr;
    if (table == nullptr) {
        return;
    }

    QTreeWidgetItem *root = nullptr;
    for (int i = 0; i < navigator->topLevelItemCount(); ++i) {
        auto *candidate = navigator->topLevelItem(i);
        if (candidate != nullptr && candidate->text(0) == QStringLiteral("Sonuçlar")) {
            root = candidate;
            break;
        }
    }
    if (root == nullptr) {
        return;
    }

    while (root->childCount() > 0) {
        delete root->takeChild(0);
    }

    QSet<QString> added;
    for (int row = 0; row < table->rowCount(); ++row) {
        const auto *item = table->item(row, 0);
        if (item == nullptr || item->text().trimmed().isEmpty()) {
            continue;
        }
        const QString name = normalizedResultObjectName(item->text().trimmed());
        if (added.contains(name)) {
            continue;
        }
        added.insert(name);
        auto *child = new QTreeWidgetItem(root, {name});
        child->setData(0, kNavigatorRoleInspectorPage, kNavigatorOpenResults);
    }
    root->setExpanded(!added.isEmpty());
}

void connectResultNavigatorSynchronization(QTreeWidget *navigator, QTabWidget *tabs, QMainWindow &window)
{
    if (navigator == nullptr || tabs == nullptr) {
        return;
    }
    const int resultTab = tabIndexContaining(tabs, QStringLiteral("Sonuç"));
    auto *table = resultTab >= 0 ? qobject_cast<QTableWidget *>(tabs->widget(resultTab)) : nullptr;
    if (table == nullptr) {
        return;
    }
    QObject::connect(table, &QTableWidget::itemChanged, &window, [navigator, tabs](QTableWidgetItem *) {
        QTimer::singleShot(0, navigator, [navigator, tabs] {
            synchronizeResultNavigator(navigator, tabs);
        });
    });
}

void replaceLegacyVisibleLogText(QMainWindow &window)
{
    const auto edits = window.findChildren<QPlainTextEdit *>();
    for (auto *edit : edits) {
        if (edit == nullptr || !edit->isReadOnly()) {
            continue;
        }
        QString text = edit->toPlainText();
        if (!text.contains(QStringLiteral("FEMCAE"))) {
            continue;
        }
        text.replace(QStringLiteral("FEMCAE"), QStringLiteral("Dynamics26"));
        edit->setPlainText(text);
    }
}

void removeLegacyToolbars(QMainWindow &window)
{
    const auto toolbars = window.findChildren<QToolBar *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *toolbar : toolbars) {
        window.removeToolBar(toolbar);
        toolbar->deleteLater();
    }
}

} // namespace

namespace dynamics26::gui {

void applyApplicationShell(QMainWindow &window)
{
    // MainWindow'ın eski light-only QSS'i constructor içinde kuruluyor. Corrective
    // shell bu global override'ı görünür UI'ya taşımadan temizler; böylece
    // Qt/macOS system palette Light/Dark Mode davranışının kaynağı olur.
    window.setStyleSheet(QString());
    window.setWindowTitle(QStringLiteral("Dynamics26"));
    window.setDocumentMode(true);
    window.setDockNestingEnabled(false);
#ifdef Q_OS_MACOS
    // QOpenGLWidget/QVTKOpenGLNativeWidget ile unified title/toolbar yolu Qt'de
    // güvenli değildir; native frame korunur ve unsupported seçenek zorlanmaz.
    window.setUnifiedTitleAndToolBarOnMac(false);
#endif

    WorkspaceParts parts = detachLegacyWorkspace(window);
    if (parts.navigator != nullptr && parts.viewport != nullptr && parts.legacyInspector != nullptr) {
        configureNavigatorWidget(parts.navigator);
        parts.viewport->setObjectName(QStringLiteral("Dynamics26Viewport"));
        parts.viewport->setMinimumWidth(520);
        parts.viewport->setAccessibleName(QStringLiteral("3D Viewport"));

        QWidget *navigatorPanel = wrapNavigator(parts.navigator);
        InspectorShell inspectorShell = wrapInspector(parts.legacyInspector);

        auto *workspace = new QSplitter(Qt::Horizontal, &window);
        workspace->setObjectName(QStringLiteral("Dynamics26WorkspaceSplitter"));
        workspace->setHandleWidth(1);
        workspace->setChildrenCollapsible(false);
        workspace->addWidget(navigatorPanel);
        workspace->addWidget(parts.viewport);
        workspace->addWidget(inspectorShell.panel);
        workspace->setStretchFactor(0, 0);
        workspace->setStretchFactor(1, 1);
        workspace->setStretchFactor(2, 0);
        workspace->setSizes({230, 960, 370});
        window.setCentralWidget(workspace);

        QDockWidget *utilityDock = nullptr;
        const auto docks = window.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
        if (!docks.isEmpty()) {
            utilityDock = docks.first();
            configureUtilityArea(utilityDock);
        }
        auto *utilityTabs = utilityDock != nullptr ? utilityDock->findChild<QTabWidget *>() : nullptr;
        connectResultNavigatorSynchronization(parts.navigator, utilityTabs, window);

        if (inspectorShell.pages != nullptr) {
            QObject::connect(
                parts.navigator,
                &QTreeWidget::currentItemChanged,
                &window,
                [inspectorShell, utilityDock, utilityTabs](QTreeWidgetItem *current, QTreeWidgetItem *) {
                    if (current == nullptr) {
                        inspectorShell.context->setText(QStringLiteral("Seçim yok"));
                        inspectorShell.pages->hide();
                        inspectorShell.emptyState->setText(
                            QStringLiteral("Proje Gezgini’nden bir öğe seçerek mühendislik özelliklerini görüntüleyin."));
                        inspectorShell.emptyState->show();
                        return;
                    }

                    inspectorShell.context->setText(current->text(0));
                    const QVariant binding = current->data(0, kNavigatorRoleInspectorPage);
                    if (!binding.isValid()) {
                        inspectorShell.pages->hide();
                        inspectorShell.emptyState->show();
                        return;
                    }

                    const int target = binding.toInt();
                    if (target == kNavigatorOpenResults) {
                        inspectorShell.pages->hide();
                        inspectorShell.emptyState->setText(
                            QStringLiteral("Sonuç değerleri ve çözüm tanıları alt utility alanında görüntülenir."));
                        inspectorShell.emptyState->show();
                        activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Sonuç"));
                        return;
                    }

                    if (target >= 0 && target < inspectorShell.pages->count()) {
                        inspectorShell.pages->setCurrentIndex(target);
                        inspectorShell.emptyState->hide();
                        inspectorShell.pages->show();
                    }
                });
        }

        // Shell-owned actions: yalnız gerçekten çalışan dosya, görünüm ve panel
        // komutları görünürdür. Verification/demo çözümleri kullanıcı komut yüzeyine
        // taşınmaz; Alpha.3 command registry gelene kadar backend yolları korunur.
        auto *newAction = makeSlotAction(
            window, QStringLiteral("Yeni"), QStringLiteral("Yeni proje oluştur"),
            "createNewProject", QStyle::SP_FileIcon);
        newAction->setShortcut(QKeySequence::New);
        QObject::connect(newAction, &QAction::triggered, &window, [navigator = parts.navigator, inspectorShell] {
            configureNavigatorWidget(navigator);
            inspectorShell.context->setText(QStringLiteral("Seçim yok"));
            inspectorShell.pages->hide();
            inspectorShell.emptyState->setText(
                QStringLiteral("Proje Gezgini’nden bir öğe seçerek mühendislik özelliklerini görüntüleyin."));
            inspectorShell.emptyState->show();
        });

        auto *openAction = makeSlotAction(
            window, QStringLiteral("Aç…"), QStringLiteral("Proje aç"),
            "openProject", QStyle::SP_DialogOpenButton);
        openAction->setShortcut(QKeySequence::Open);

        auto *saveAction = makeSlotAction(
            window, QStringLiteral("Kaydet"), QStringLiteral("Projeyi kaydet"),
            "saveProject", QStyle::SP_DialogSaveButton);
        saveAction->setShortcut(QKeySequence::Save);

        auto *fitAction = makeSlotAction(
            window, QStringLiteral("Görünüme Sığdır"), QStringLiteral("Modeli 3D viewport'a sığdır"),
            "resetView", QStyle::SP_BrowserReload);

        auto *navigatorAction = new QAction(QStringLiteral("Proje Gezgini"), &window);
        navigatorAction->setCheckable(true);
        navigatorAction->setChecked(true);
        navigatorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
        navigatorAction->setToolTip(QStringLiteral("Proje Gezgini’ni göster veya gizle (⌘1)"));
        setStandardIcon(window, navigatorAction, QStyle::SP_FileDialogListView);
        QObject::connect(navigatorAction, &QAction::toggled, navigatorPanel, &QWidget::setVisible);

        auto *inspectorAction = new QAction(QStringLiteral("Özellikler"), &window);
        inspectorAction->setCheckable(true);
        inspectorAction->setChecked(true);
        inspectorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
        inspectorAction->setToolTip(QStringLiteral("Özellikler panelini göster veya gizle (⌘2)"));
        setStandardIcon(window, inspectorAction, QStyle::SP_FileDialogDetailedView);
        QObject::connect(inspectorAction, &QAction::toggled, inspectorShell.panel, &QWidget::setVisible);

        auto *utilityAction = new QAction(QStringLiteral("Sonuçlar ve Tanılama"), &window);
        utilityAction->setCheckable(true);
        utilityAction->setChecked(false);
        utilityAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
        utilityAction->setToolTip(QStringLiteral("Sonuçlar, Yakınsama ve Günlük alanını göster veya gizle (⌘J)"));
        setStandardIcon(window, utilityAction, QStyle::SP_FileDialogContentsView);
        QObject::connect(utilityAction, &QAction::toggled, &window, [utilityDock](bool visible) {
            if (utilityDock != nullptr) {
                utilityDock->setVisible(visible);
            }
        });
        if (utilityDock != nullptr) {
            QObject::connect(utilityDock, &QDockWidget::visibilityChanged, utilityAction, &QAction::setChecked);
        }

        auto *aboutAction = new QAction(QStringLiteral("Dynamics26 Hakkında"), &window);
        aboutAction->setMenuRole(QAction::AboutRole);
        QObject::connect(aboutAction, &QAction::triggered, &window, [&window] {
            QMessageBox::about(
                &window,
                QStringLiteral("Dynamics26"),
                QStringLiteral("Dynamics26\nGUI milestone %1\nEngine %2.%3.%4\nC API %5\n\nmacOS odaklı modern FEA/CAE platformu.")
                    .arg(QStringLiteral(DYNAMICS26_GUI_MILESTONE))
                    .arg(fem_version_major()).arg(fem_version_minor()).arg(fem_version_patch())
                    .arg(fem_api_version()));
        });

        auto *quitAction = new QAction(QStringLiteral("Dynamics26’dan Çık"), &window);
        quitAction->setMenuRole(QAction::QuitRole);
        quitAction->setShortcut(QKeySequence::Quit);
        QObject::connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

        QMenuBar *bar = window.menuBar();
        bar->clear();
#ifdef Q_OS_MACOS
        bar->setNativeMenuBar(true);
#endif
        auto *fileMenu = bar->addMenu(QStringLiteral("Dosya"));
        fileMenu->addAction(newAction);
        fileMenu->addAction(openAction);
        fileMenu->addAction(saveAction);
        fileMenu->addSeparator();
        fileMenu->addAction(quitAction);

        auto *viewMenu = bar->addMenu(QStringLiteral("Görünüm"));
        viewMenu->addAction(navigatorAction);
        viewMenu->addAction(inspectorAction);
        viewMenu->addAction(utilityAction);
        viewMenu->addSeparator();
        viewMenu->addAction(fitAction);

        auto *helpMenu = bar->addMenu(QStringLiteral("Yardım"));
        helpMenu->addAction(aboutAction);

        removeLegacyToolbars(window);
        auto *mainToolbar = new QToolBar(QStringLiteral("Ana Araç Çubuğu"), &window);
        mainToolbar->setObjectName(QStringLiteral("Dynamics26MainToolbar"));
        mainToolbar->setMovable(false);
        mainToolbar->setFloatable(false);
        mainToolbar->setAllowedAreas(Qt::TopToolBarArea);
        mainToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mainToolbar->setIconSize(QSize(16, 16));
        mainToolbar->setMinimumHeight(32);
        mainToolbar->setMaximumHeight(36);
        if (mainToolbar->layout() != nullptr) {
            mainToolbar->layout()->setContentsMargins(4, 0, 4, 0);
            mainToolbar->layout()->setSpacing(4);
        }
        window.addToolBar(Qt::TopToolBarArea, mainToolbar);

        mainToolbar->addAction(navigatorAction);
        mainToolbar->addAction(fitAction);
        mainToolbar->addWidget(makeExpandingSpacer(mainToolbar));
        mainToolbar->addAction(utilityAction);
        mainToolbar->addAction(inspectorAction);

        if (utilityDock != nullptr) {
            utilityDock->setVisible(false);
            // QMainWindow henüz show edilmeden shell uygulanabildiği için sıfır
            // gecikmeli ikinci hide gerçek ilk macOS frame'inde açık debug-dock
            // görünmesini engeller. Results seçimi veya ⌘J sonradan bilinçli açar.
            QTimer::singleShot(0, utilityDock, [utilityDock] {
                utilityDock->hide();
            });
        }
    }

    replaceLegacyVisibleLogText(window);

    // Engine/API sürümleri About/Diagnostics içeriğidir; kalıcı status bar
    // viewport alanını tüketmez.
    window.statusBar()->clearMessage();
    window.statusBar()->hide();
}

} // namespace dynamics26::gui
