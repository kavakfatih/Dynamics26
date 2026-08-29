#include "Alpha1UxController.h"

#include "GeometryPanel.h"
#include "PrePostPanel.h"

#include <QComboBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QMetaObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

namespace {

QTreeWidgetItem *topLevelRoot(QTreeWidgetItem *item)
{
    if (item == nullptr) {
        return nullptr;
    }
    while (item->parent() != nullptr) {
        item = item->parent();
    }
    return item;
}

int utilityTabIndex(QTabWidget *tabs, const QString &needle)
{
    if (tabs == nullptr) {
        return -1;
    }
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i).contains(needle, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

void showUtilityTab(QDockWidget *dock, QTabWidget *tabs, const QString &needle)
{
    if (dock == nullptr || tabs == nullptr) {
        return;
    }
    const int index = utilityTabIndex(tabs, needle);
    if (index >= 0) {
        tabs->setCurrentIndex(index);
    }
    dock->show();
}

QComboBox *findAnalysisTypeCombo(QMainWindow &window)
{
    const auto combos = window.findChildren<QComboBox *>();
    for (auto *combo : combos) {
        if (combo->findText(QStringLiteral("Linear Static")) >= 0
            && combo->findText(QStringLiteral("Modal / Free Vibration")) >= 0) {
            return combo;
        }
    }
    return nullptr;
}

QComboBox *findModeSelector(QWidget *page)
{
    if (page == nullptr) {
        return nullptr;
    }
    const auto combos = page->findChildren<QComboBox *>();
    for (auto *combo : combos) {
        if (combo->findText(QStringLiteral("Mode 1")) >= 0
            && combo->findText(QStringLiteral("Mode 2")) >= 0) {
            return combo;
        }
    }
    return nullptr;
}

void polishGeometryInspector(GeometryPanel *panel)
{
    if (panel == nullptr) {
        return;
    }

    const auto groups = panel->findChildren<QGroupBox *>();
    for (auto *group : groups) {
        group->setFlat(true);
        if (group->title().contains(QStringLiteral("CAD Geometry"), Qt::CaseInsensitive)) {
            group->setTitle(QStringLiteral("CAD Geometrisi"));
        } else if (group->title().contains(QStringLiteral("Custom Section"), Qt::CaseInsensitive)) {
            group->setTitle(QStringLiteral("Özel Kesit / DXF"));
        }
    }

    const auto labels = panel->findChildren<QLabel *>();
    for (auto *label : labels) {
        if (label->text().contains(QStringLiteral("CAD B-Rep"), Qt::CaseInsensitive)) {
            // Mimari invariant korunur ancak normal Inspector'da debug/dokümantasyon
            // paragrafı olarak sürekli yer kaplamaz. Ayrıntı Diagnostics/docs katmanındadır.
            label->hide();
        }
    }
}

void normalizeMeshSummary(PrePostPanel *panel)
{
    if (panel == nullptr) {
        return;
    }

    static const QRegularExpression qualityPattern(
        QStringLiteral("Nodes=(\\d+)\\s+HEX8=(\\d+)\\s+Boundary facets=(\\d+)\\n"
                       "min scaled-J=([^\\s]+)\\s+max aspect=([^\\s]+)\\s+inverted=(\\d+)"));

    const auto labels = panel->findChildren<QLabel *>();
    for (auto *label : labels) {
        if (label->text() == QStringLiteral("Henüz FEM mesh yok.")) {
            label->setText(QStringLiteral("Henüz mesh oluşturulmadı."));
            continue;
        }
        const auto match = qualityPattern.match(label->text());
        if (!match.hasMatch()) {
            continue;
        }
        label->setText(
            QStringLiteral("Düğümler: %1    Elemanlar: %2\n"
                           "Kalite — min Scaled Jacobian: %3  •  Maks. Aspect: %4  •  Ters eleman: %5")
                .arg(match.captured(1), match.captured(2), match.captured(4), match.captured(5), match.captured(6)));
    }
}

void polishMeshInspector(PrePostPanel *panel)
{
    if (panel == nullptr) {
        return;
    }

    const auto groups = panel->findChildren<QGroupBox *>();
    for (auto *group : groups) {
        group->setFlat(true);
        if (group->title().contains(QStringLiteral("Structured HEX8 Mesh"), Qt::CaseInsensitive)) {
            group->setTitle(QStringLiteral("Structured HEX8"));
        } else if (group->title().contains(QStringLiteral("Geometry Assignment"), Qt::CaseInsensitive)) {
            // Mesh Inspector yalnız mesh tanımını ve kalite özetini taşır. Çözüm
            // komutu Analysis Inspector'a yönlendirilir; material/load/post alanları
            // Mesh bağlamında görünmez.
            group->hide();
        }
    }

    const auto buttons = panel->findChildren<QPushButton *>();
    for (auto *button : buttons) {
        const QString text = button->text();
        if (text.contains(QStringLiteral("CSV"), Qt::CaseInsensitive)
            || text.contains(QStringLiteral("VTK"), Qt::CaseInsensitive)
            || text.contains(QStringLiteral("Kesit Probe"), Qt::CaseInsensitive)) {
            button->hide();
        }
    }

    const auto labels = panel->findChildren<QLabel *>();
    for (auto *label : labels) {
        const QString text = label->text();
        if (text.contains(QStringLiteral("Section cut"), Qt::CaseInsensitive)
            || text.contains(QStringLiteral("Bu akış display tessellation"), Qt::CaseInsensitive)) {
            label->hide();
        }
    }

    normalizeMeshSummary(panel);
}

void normalizeResultTable(QTabWidget *tabs)
{
    if (tabs == nullptr) {
        return;
    }
    const int index = utilityTabIndex(tabs, QStringLiteral("Sonuç"));
    auto *table = index >= 0 ? qobject_cast<QTableWidget *>(tabs->widget(index)) : nullptr;
    if (table == nullptr) {
        return;
    }

    for (int row = 0; row < table->rowCount(); ++row) {
        auto *item = table->item(row, 0);
        if (item == nullptr) {
            continue;
        }
        const QString source = item->text();
        if (source.contains(QStringLiteral("Mesh max |u|"), Qt::CaseInsensitive)) {
            item->setText(QStringLiteral("Maksimum Toplam Deformasyon"));
        } else if (source.contains(QStringLiteral("Mesh max von Mises"), Qt::CaseInsensitive)) {
            item->setText(QStringLiteral("Maksimum von Mises Gerilmesi"));
        } else if (source.contains(QStringLiteral("ΣRx"), Qt::CaseInsensitive)) {
            item->setText(QStringLiteral("Toplam Reaksiyon Kuvveti X"));
        } else if (source.contains(QStringLiteral("Probe Node"), Qt::CaseInsensitive)) {
            item->setText(QStringLiteral("Probe Düğümü"));
        } else if (source.contains(QStringLiteral("Probe ux"), Qt::CaseInsensitive)) {
            item->setText(QStringLiteral("Probe Deplasmanı X"));
        }
    }
}

void configureAnalysisContext(
    QMainWindow &window,
    PrePostPanel *prePost,
    QDockWidget *utilityDock,
    QTabWidget *utilityTabs,
    QStatusBar *status)
{
    QComboBox *analysisType = findAnalysisTypeCombo(window);
    if (analysisType == nullptr) {
        return;
    }

    QWidget *page = analysisType->parentWidget();
    auto *form = page != nullptr ? qobject_cast<QFormLayout *>(page->layout()) : nullptr;
    if (page == nullptr || form == nullptr) {
        return;
    }

    analysisType->setItemText(0, QStringLiteral("Lineer Statik"));
    analysisType->setItemText(1, QStringLiteral("Modal / Serbest Titreşim"));
    analysisType->setItemText(2, QStringLiteral("Nonlineer Statik / Büyük Deformasyon"));

    QComboBox *modeSelector = findModeSelector(page);
    QWidget *modeLabel = modeSelector != nullptr ? form->labelForField(modeSelector) : nullptr;

    QLabel *nonlinearHeading = nullptr;
    const auto labels = page->findChildren<QLabel *>();
    for (auto *label : labels) {
        if (label->text().contains(QStringLiteral("Nonlineer Statik / Büyük Deformasyon"), Qt::CaseInsensitive)) {
            nonlinearHeading = label;
            break;
        }
    }

    auto *advanced = page->findChild<QToolButton *>(QStringLiteral("Dynamics26AdvancedSolverDisclosure"));

    auto *solveButton = page->findChild<QPushButton *>(QStringLiteral("Dynamics26IntegratedSolve"));
    if (solveButton == nullptr) {
        solveButton = new QPushButton(QStringLiteral("Çöz"), page);
        solveButton->setObjectName(QStringLiteral("Dynamics26IntegratedSolve"));
        solveButton->setToolTip(QStringLiteral("Seçili lineer modeli çöz"));
        form->insertRow(1, solveButton);
    }

    if (prePost != nullptr) {
        QObject::connect(solveButton, &QPushButton::clicked, prePost, [prePost, status] {
            if (status != nullptr) {
                status->showMessage(QStringLiteral("Çözülüyor…"));
            }
            if (!QMetaObject::invokeMethod(prePost, "solveLinear", Qt::QueuedConnection)
                && status != nullptr) {
                status->showMessage(QStringLiteral("Çöz komutu engineering katmanına bağlanamadı."));
            }
        });
    }

    const auto updateVisibility = [analysisType, modeSelector, modeLabel, nonlinearHeading, advanced, solveButton] {
        const int index = analysisType->currentIndex();
        const bool modal = index == 1;
        const bool nonlinear = index == 2;
        const bool integratedLinear = index == 0;

        if (modeSelector != nullptr) {
            modeSelector->setVisible(modal);
        }
        if (modeLabel != nullptr) {
            modeLabel->setVisible(modal);
        }
        if (nonlinearHeading != nullptr) {
            nonlinearHeading->setVisible(nonlinear);
        }
        if (advanced != nullptr) {
            if (!nonlinear) {
                advanced->setChecked(false);
            }
            advanced->setVisible(nonlinear);
        }

        solveButton->setEnabled(integratedLinear);
        solveButton->setToolTip(integratedLinear
            ? QStringLiteral("Seçili lineer modeli çöz")
            : QStringLiteral("Bu analiz türünün entegre GUI workflow bağlantısı sonraki aşamada tamamlanacak; solver doğrulama yolu backend'de korunuyor."));
    };

    QObject::connect(analysisType, qOverload<int>(&QComboBox::currentIndexChanged), page,
                     [updateVisibility](int) { updateVisibility(); });
    updateVisibility();

    Q_UNUSED(utilityDock)
    Q_UNUSED(utilityTabs)
}

void configureEngineeringStatusStrip(
    QMainWindow &window,
    PrePostPanel *prePost,
    QDockWidget *utilityDock,
    QTabWidget *utilityTabs)
{
    QStatusBar *status = window.statusBar();
    if (status == nullptr) {
        return;
    }

    status->clearMessage();
    status->setSizeGripEnabled(false);
    status->setMinimumHeight(22);
    status->setMaximumHeight(24);
    status->showMessage(QStringLiteral("Hazır"));
    status->show();

    auto *diagnostics = status->findChild<QToolButton *>(QStringLiteral("Dynamics26DiagnosticsHandle"));
    if (diagnostics == nullptr) {
        diagnostics = new QToolButton(status);
        diagnostics->setObjectName(QStringLiteral("Dynamics26DiagnosticsHandle"));
        diagnostics->setText(QStringLiteral("Tanılama"));
        diagnostics->setAutoRaise(true);
        diagnostics->setCheckable(true);
        diagnostics->setToolTip(QStringLiteral("Sonuçlar, Yakınsama ve Günlük çekmecesini göster veya gizle"));
        status->addPermanentWidget(diagnostics);
    }

    if (utilityDock != nullptr) {
        // Klasik QDockWidget title chrome'u yerine alttaki engineering status strip
        // çekmecenin görünür tutamağıdır. İçerik açıldığında aynı veri/sekme korunur.
        auto *minimalTitleBar = new QWidget(utilityDock);
        minimalTitleBar->setFixedHeight(1);
        utilityDock->setTitleBarWidget(minimalTitleBar);

        diagnostics->setChecked(utilityDock->isVisible());
        QObject::connect(diagnostics, &QToolButton::toggled, utilityDock, &QWidget::setVisible);
        QObject::connect(utilityDock, &QDockWidget::visibilityChanged, diagnostics, &QToolButton::setChecked);
    }

    if (prePost != nullptr) {
        QObject::connect(prePost, &PrePostPanel::message, status,
            [status, utilityDock, utilityTabs, prePost](const QString &message) {
                static const QRegularExpression meshPattern(
                    QStringLiteral("structured mesh oluşturuldu: (\\d+) node, (\\d+) HEX8"),
                    QRegularExpression::CaseInsensitiveOption);
                const auto match = meshPattern.match(message);
                if (match.hasMatch()) {
                    status->showMessage(
                        QStringLiteral("%1 düğüm • %2 HEX8 • Hazır")
                            .arg(match.captured(1), match.captured(2)));
                    normalizeMeshSummary(prePost);
                    return;
                }
                if (message.contains(QStringLiteral("başarısız"), Qt::CaseInsensitive)
                    || message.contains(QStringLiteral("hata"), Qt::CaseInsensitive)) {
                    status->showMessage(QStringLiteral("Hata • %1").arg(message));
                    showUtilityTab(utilityDock, utilityTabs, QStringLiteral("Günlük"));
                }
            });

        QObject::connect(prePost, &PrePostPanel::solveCompleted, status,
            [status, utilityDock, utilityTabs](double maxU, double maxVm, double, qlonglong, double) {
                status->showMessage(
                    QStringLiteral("Çözüm tamamlandı • umax %1 mm • von Mises %2 MPa")
                        .arg(maxU, 0, 'g', 5)
                        .arg(maxVm, 0, 'g', 5));
                normalizeResultTable(utilityTabs);
                showUtilityTab(utilityDock, utilityTabs, QStringLiteral("Sonuç"));
            });
    }
}

} // namespace

namespace dynamics26::gui {

void attachAlpha1UxController(QMainWindow &window)
{
    auto *navigator = window.findChild<QTreeWidget *>(QStringLiteral("Dynamics26Navigator"));
    auto *geometry = window.findChild<GeometryPanel *>();
    auto *prePost = window.findChild<PrePostPanel *>();
    auto *utilityDock = window.findChild<QDockWidget *>(QStringLiteral("Dynamics26UtilityArea"));
    auto *utilityTabs = utilityDock != nullptr
        ? utilityDock->findChild<QTabWidget *>(QStringLiteral("Dynamics26UtilityTabs"))
        : nullptr;

    polishGeometryInspector(geometry);
    polishMeshInspector(prePost);
    configureEngineeringStatusStrip(window, prePost, utilityDock, utilityTabs);
    configureAnalysisContext(window, prePost, utilityDock, utilityTabs, window.statusBar());
    normalizeResultTable(utilityTabs);

    if (navigator != nullptr) {
        QObject::connect(
            navigator,
            &QTreeWidget::currentItemChanged,
            &window,
            [geometry, prePost](QTreeWidgetItem *current, QTreeWidgetItem *) {
                QTreeWidgetItem *root = topLevelRoot(current);
                if (root == nullptr) {
                    return;
                }

                const QString context = root->text(0);
                if (context == QStringLiteral("Sonuçlar")) {
                    if (prePost != nullptr && !prePost->showResultsPreview()) {
                        (void)prePost->showMeshPreview();
                    }
                    return;
                }

                if (context == QStringLiteral("Geometri")) {
                    if (geometry != nullptr && geometry->showCurrentGeometry()) {
                        return;
                    }
                    if (prePost != nullptr) {
                        (void)prePost->showMeshPreview();
                    }
                    return;
                }

                // Material, Section, Mesh, Load/BC ve Analysis preprocessing
                // bağlamlarında result contour gösterilmez. Mesh varsa nötr mesh,
                // yoksa mevcut CAD geometry preview korunur/yeniden gösterilir.
                if (prePost != nullptr && prePost->showMeshPreview()) {
                    return;
                }
                if (geometry != nullptr) {
                    (void)geometry->showCurrentGeometry();
                }
            });
    }

    if (prePost != nullptr) {
        QObject::connect(
            prePost,
            &PrePostPanel::solveCompleted,
            &window,
            [navigator, prePost](double, double, double, qlonglong, double) {
                // Solve sonrasında result database saklanır fakat kullanıcı Results
                // bağlamında değilse contour preprocessing görünümüne sızmaz.
                QTreeWidgetItem *root = navigator != nullptr
                    ? topLevelRoot(navigator->currentItem())
                    : nullptr;
                if (root == nullptr || root->text(0) != QStringLiteral("Sonuçlar")) {
                    (void)prePost->showMeshPreview();
                }
            });
    }
}

} // namespace dynamics26::gui
