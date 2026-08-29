#pragma once

#include <QSignalBlocker>

class QComboBox;
class QMainWindow;

// CaeWorkbenchController.cpp aynı translation unit içinde bu helper'ı internal
// linkage ile tanımlar. Declaration burada yalnız controller derlemesindeki
// compact Details -> legacy analysis-state köprüsünü önden görünür kılar.
namespace {
QComboBox *findAnalysisTypeCombo(QMainWindow &window);
}

namespace dynamics26::gui {

// Alpha.1 CAE workbench controller.
//
// Bu katman görünür Dynamics26 workstation davranışını tek yerde toplar:
// Model Tree -> Graphics -> Details -> Utility Workspace. Backend geometri,
// mesh, solver veya result verisinin sahibi değildir; mevcut engineering
// widget/service yollarını bağlar. Alpha.2'de gerçek ProjectTreeModel bu
// display-only ağaç sözleşmesinin yerini alacaktır.
void installCaeWorkbenchController(QMainWindow &window);

} // namespace dynamics26::gui
