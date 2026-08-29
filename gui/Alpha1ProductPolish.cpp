#include "Alpha1ProductPolish.h"

#include "GeometryPanel.h"
#include "PrePostPanel.h"

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMetaObject>
#include <QPushButton>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QWidget>

namespace {

QAction *toolbarActionContaining(QToolBar *toolbar, const QString &text)
{
    if (toolbar == nullptr) {
        return nullptr;
    }
    for (auto *action : toolbar->actions()) {
        if (action != nullptr && action->text().contains(text, Qt::CaseInsensitive)) {
            return action;
        }
    }
    return nullptr;
}

QComboBox *findAnalysisTypeCombo(QMainWindow &window)
{
    const auto combos = window.findChildren<QComboBox *>();
    for (auto *combo : combos) {
        if (combo == nullptr) {
            continue;
        }
        const bool linear = combo->findText(QStringLiteral("Lineer Statik")) >= 0
            || combo->findText(QStringLiteral("Linear Static")) >= 0;
        const bool modal = combo->findText(QStringLiteral("Modal / Serbest Titreşim")) >= 0
            || combo->findText(QStringLiteral("Modal / Free Vibration")) >= 0;
        if (linear && modal) {
            return combo;
        }
    }
    return nullptr;
}

QComboBox *findFormulationCombo(QMainWindow &window)
{
    const auto combos = window.findChildren<QComboBox *>();
    for (auto *combo : combos) {
        if (combo == nullptr) {
            continue;
        }
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemText(i).contains(QStringLiteral("Displacement-only"), Qt::CaseInsensitive)
                || combo->itemText(i).contains(QStringLiteral("Mixed u-p"), Qt::CaseInsensitive)) {
                return combo;
            }
        }
    }
    return nullptr;
}

void selectTopLevel(QTreeWidget *navigator, const QString &name)
{
    if (navigator == nullptr) {
        return;
    }
    for (int i = 0; i < navigator->topLevelItemCount(); ++i) {
        auto *item = navigator->topLevelItem(i);
        if (item != nullptr && item->text(0) == name) {
            navigator->setCurrentItem(item);
            return;
        }
    }
}

QAction *makeEngineeringAction(
    QMainWindow &window,
    const QString &text,
    const QString &toolTip,
    QStyle::StandardPixmap icon)
{
    auto *action = new QAction(text, &window);
    action->setToolTip(toolTip);
    if (window.style() != nullptr) {
        action->setIcon(window.style()->standardIcon(icon));
    }
    return action;
}

void configureCompactCaeToolbar(QMainWindow &window)
{
    auto *toolbar = window.findChild<QToolBar *>(QStringLiteral("Dynamics26MainToolbar"));
    if (toolbar == nullptr) {
        return;
    }

    auto *navigator = window.findChild<QTreeWidget *>(QStringLiteral("Dynamics26Navigator"));
    auto *geometry = window.findChild<GeometryPanel *>();
    auto *prePost = window.findChild<PrePostPanel *>();
    auto *analysisType = findAnalysisTypeCombo(window);
    auto *integratedSolve = window.findChild<QPushButton *>(QStringLiteral("Dynamics26IntegratedSolve"));

    QAction *navigatorAction = toolbarActionContaining(toolbar, QStringLiteral("Proje Gezgini"));
    QAction *fitAction = toolbarActionContaining(toolbar, QStringLiteral("Sığdır"));
    QAction *diagnosticsAction = toolbarActionContaining(toolbar, QStringLiteral("Tanılama"));
    if (diagnosticsAction == nullptr) {
        diagnosticsAction = toolbarActionContaining(toolbar, QStringLiteral("Sonuçlar ve Tanılama"));
    }
    QAction *inspectorAction = toolbarActionContaining(toolbar, QStringLiteral("Özellikler"));

    auto *importAction = makeEngineeringAction(
        window,
        QStringLiteral("Geometri İçe Aktar"),
        QStringLiteral("STEP / STP geometrisi içe aktar"),
        QStyle::SP_DialogOpenButton);
    importAction->setObjectName(QStringLiteral("Dynamics26ImportGeometryAction"));
    importAction->setEnabled(geometry != nullptr);
    QObject::connect(importAction, &QAction::triggered, &window, [geometry, navigator] {
        selectTopLevel(navigator, QStringLiteral("Geometri"));
        if (geometry != nullptr) {
            QMetaObject::invokeMethod(geometry, "importStep", Qt::QueuedConnection);
        }
    });

    auto *meshAction = makeEngineeringAction(
        window,
        QStringLiteral("Mesh Oluştur"),
        QStringLiteral("Structured HEX8 mesh oluştur"),
        QStyle::SP_ComputerIcon);
    meshAction->setObjectName(QStringLiteral("Dynamics26GenerateMeshAction"));
    meshAction->setEnabled(prePost != nullptr);
    QObject::connect(meshAction, &QAction::triggered, &window, [prePost, navigator] {
        selectTopLevel(navigator, QStringLiteral("Mesh"));
        if (prePost != nullptr) {
            QMetaObject::invokeMethod(prePost, "generateMesh", Qt::QueuedConnection);
        }
    });

    auto *solveAction = makeEngineeringAction(
        window,
        QStringLiteral("Çöz"),
        QStringLiteral("Seçili lineer analizi çöz"),
        QStyle::SP_MediaPlay);
    solveAction->setObjectName(QStringLiteral("Dynamics26SolveAction"));
    QObject::connect(solveAction, &QAction::triggered, &window, [integratedSolve, navigator] {
        selectTopLevel(navigator, QStringLiteral("Analizler"));
        if (integratedSolve != nullptr && integratedSolve->isEnabled()) {
            integratedSolve->click();
        }
    });

    const auto refreshSolveAvailability = [solveAction, analysisType, integratedSolve] {
        const bool linear = analysisType != nullptr && analysisType->currentIndex() == 0;
        const bool available = linear && integratedSolve != nullptr && integratedSolve->isEnabled();
        solveAction->setEnabled(available);
        solveAction->setToolTip(available
            ? QStringLiteral("Seçili lineer analizi çöz")
            : QStringLiteral("Bu analiz türü henüz GUI Çöz akışına bağlı değil."));
    };
    refreshSolveAvailability();
    if (analysisType != nullptr) {
        QObject::connect(analysisType, qOverload<int>(&QComboBox::currentIndexChanged), &window,
                         [refreshSolveAvailability](int) { refreshSolveAvailability(); });
    }

    // Alpha.1 toolbar'ı command warehouse değildir. Yalnız mevcut backend'e
    // bağlı, sık kullanılan global CAE komutları tutulur. Alpha.3 Context Bar
    // gelene kadar Geometry/Mesh/Results'e özgü ayrıntılı araçlar eklenmez.
    toolbar->clear();
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setMinimumHeight(34);
    toolbar->setMaximumHeight(36);

    if (navigatorAction != nullptr) toolbar->addAction(navigatorAction);
    toolbar->addSeparator();
    toolbar->addAction(importAction);
    toolbar->addAction(meshAction);
    if (fitAction != nullptr) toolbar->addAction(fitAction);

    auto *spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Solve, uygulamanın tek birincil global eylemidir; bu nedenle toolbar'da
    // kısa metin etiketi taşıyan tek kontrol olarak bırakılır.
    auto *solveButton = new QToolButton(toolbar);
    solveButton->setObjectName(QStringLiteral("Dynamics26ToolbarSolveButton"));
    solveButton->setDefaultAction(solveAction);
    solveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    solveButton->setAutoRaise(false);
    toolbar->addWidget(solveButton);

    toolbar->addSeparator();
    if (diagnosticsAction != nullptr) toolbar->addAction(diagnosticsAction);
    if (inspectorAction != nullptr) toolbar->addAction(inspectorAction);
}

void configureAnalysisProductLanguage(QMainWindow &window)
{
    auto *analysisType = findAnalysisTypeCombo(window);
    auto *solveButton = window.findChild<QPushButton *>(QStringLiteral("Dynamics26IntegratedSolve"));
    if (analysisType == nullptr || solveButton == nullptr) {
        return;
    }

    QWidget *page = analysisType->parentWidget();
    auto *form = page != nullptr ? qobject_cast<QFormLayout *>(page->layout()) : nullptr;
    if (page == nullptr || form == nullptr) {
        return;
    }

    auto *reason = page->findChild<QLabel *>(QStringLiteral("Dynamics26SolveAvailabilityReason"));
    if (reason == nullptr) {
        reason = new QLabel(page);
        reason->setObjectName(QStringLiteral("Dynamics26SolveAvailabilityReason"));
        reason->setWordWrap(true);
        reason->setForegroundRole(QPalette::PlaceholderText);
        QFont font = reason->font();
        font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.0));
        reason->setFont(font);
        form->insertRow(2, reason);
    }

    auto *formulation = findFormulationCombo(window);
    if (formulation != nullptr) {
        for (int i = 0; i < formulation->count(); ++i) {
            const QString item = formulation->itemText(i);
            if (item.contains(QStringLiteral("Displacement-only"), Qt::CaseInsensitive)) {
                formulation->setItemText(i, QStringLiteral("Standart (displacement-based)"));
            } else if (item.contains(QStringLiteral("Mixed u-p"), Qt::CaseInsensitive)) {
                formulation->setItemText(i, QStringLiteral("Nearly Incompressible (mixed u-p)"));
            } else if (item.contains(QStringLiteral("Contact / Friction"), Qt::CaseInsensitive)) {
                formulation->setItemText(i, QStringLiteral("Contact / Friction (doğrulama)"));
            }
        }
    }

    const auto refresh = [analysisType, reason] {
        if (analysisType->currentIndex() == 0) {
            reason->clear();
            reason->hide();
        } else if (analysisType->currentIndex() == 1) {
            reason->setText(QStringLiteral("Modal çözüm GUI akışı bu Alpha.1 preview'da henüz etkin değil."));
            reason->show();
        } else {
            reason->setText(QStringLiteral("Nonlineer çözüm GUI akışı bu Alpha.1 preview'da henüz etkin değil."));
            reason->show();
        }
    };
    refresh();
    QObject::connect(analysisType, qOverload<int>(&QComboBox::currentIndexChanged), page,
                     [refresh](int) { refresh(); });
}

void enforceCollapsedUtilityStartup(QMainWindow &window)
{
    auto *dock = window.findChild<QDockWidget *>(QStringLiteral("Dynamics26UtilityArea"));
    if (dock == nullptr) {
        return;
    }

    // Alpha.1 başlangıç durumu viewport-first'tür. Drawer solve/error/Results
    // veya kullanıcının Tanılama komutuyla sonradan açılır; boş tablo başlangıçta
    // 3D çalışma alanını tüketmez.
    dock->hide();
    QTimer::singleShot(0, &window, [dock] {
        if (dock != nullptr) {
            dock->hide();
        }
    });
}

} // namespace

namespace dynamics26::gui {

void installAlpha1ProductPolish(QMainWindow &window)
{
    configureCompactCaeToolbar(window);
    configureAnalysisProductLanguage(window);
    enforceCollapsedUtilityStartup(window);
}

} // namespace dynamics26::gui
