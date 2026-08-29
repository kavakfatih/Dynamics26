#pragma once

#include <QMainWindow>

#include <array>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QSpinBox;
class QTableWidget;
class QTimer;
class QTreeWidget;
class ViewportWidget;
class MaterialCurveWidget;
class GeometryPanel;
class PrePostPanel;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void runLinearDemo();
    void runModalDemo();
    void runNonlinearDemo();
    void animateModalMode();
    void modalSelectionChanged(int index);
    void materialModelChanged(int index);
    void runMaterialPreview();
    void resetView();
    void createNewProject();
    void openProject();
    void saveProject();

private:
    QWidget *createInspector();
    QWidget *createMaterialEditor();
    QWidget *createSectionEditor();
    QWidget *createLoadBcEditor();
    QWidget *createAnalysisEditor();
    void buildModelTree();
    void updateResultTree(double displacementMm, double stressMPa, double reactionN);
    void updateModalResultTree();
    void updateNonlinearResultTree(double displacementMm, double loadFactor, int steps, int iterations, int cutbacks);
    void showSelectedModalFrame(double phase);
    void appendLog(const QString &message);
    void applyMacStyle();

    QTreeWidget *modelTree_ = nullptr;
    ViewportWidget *viewport_ = nullptr;
    QPlainTextEdit *log_ = nullptr;
    QTableWidget *results_ = nullptr;
    QTableWidget *convergenceHistory_ = nullptr;


    QComboBox *materialModel_ = nullptr;
    QDoubleSpinBox *bulkMPa_ = nullptr;
    QDoubleSpinBox *c10MPa_ = nullptr;
    QDoubleSpinBox *c01MPa_ = nullptr;
    QDoubleSpinBox *c20MPa_ = nullptr;
    QDoubleSpinBox *c30MPa_ = nullptr;
    QSpinBox *ogdenTerms_ = nullptr;
    std::array<QDoubleSpinBox *, 3> ogdenMuMPa_ {nullptr, nullptr, nullptr};
    std::array<QDoubleSpinBox *, 3> ogdenAlpha_ {nullptr, nullptr, nullptr};
    QLabel *materialValidation_ = nullptr;
    MaterialCurveWidget *materialCurve_ = nullptr;
    GeometryPanel *geometryPanel_ = nullptr;
    PrePostPanel *prePostPanel_ = nullptr;
    QDoubleSpinBox *youngGPa_ = nullptr;
    QDoubleSpinBox *poisson_ = nullptr;
    QDoubleSpinBox *densityKgM3_ = nullptr;
    QDoubleSpinBox *areaMm2_ = nullptr;
    QDoubleSpinBox *lengthMm_ = nullptr;
    QDoubleSpinBox *forceN_ = nullptr;
    QLabel *solveSummary_ = nullptr;
    QLabel *modalSummary_ = nullptr;
    QComboBox *modeSelector_ = nullptr;
    QComboBox *nonlinearMethod_ = nullptr;
    QComboBox *nonlinearFormulation_ = nullptr;
    QDoubleSpinBox *mixedShearGamma_ = nullptr;
    QComboBox *contactEnforcement_ = nullptr;
    QDoubleSpinBox *contactPenaltyFactor_ = nullptr;
    QDoubleSpinBox *nonlinearInitialIncrement_ = nullptr;
    QDoubleSpinBox *nonlinearMinimumIncrement_ = nullptr;
    QDoubleSpinBox *nonlinearMaximumIncrement_ = nullptr;
    QSpinBox *nonlinearMaxIterations_ = nullptr;
    QCheckBox *nonlinearLineSearch_ = nullptr;
    QCheckBox *nonlinearAdaptive_ = nullptr;
    QLabel *nonlinearSummary_ = nullptr;
    QTimer *modalTimer_ = nullptr;

    std::array<double, 2> modalFrequenciesHz_ {0.0, 0.0};
    std::array<double, 2> modalMid_ {0.0, 0.0};
    std::array<double, 2> modalTip_ {0.0, 0.0};
    double modalPhase_ = 0.0;
    bool modalReady_ = false;
};
