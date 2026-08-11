#include "inspector.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

#include <limits>

namespace {

using FieldMeta = Shit::FieldMeta;

/// 取字段的编辑元数据（meta 可多个，用第一个非空 displayName 的，否则空）
const FieldMeta* metaOf(const Shit::FieldInfo &field)
{
    return field.meta.empty() ? nullptr : &field.meta[0];
}

/// 类型名归一化：引擎头内声明为 "Vector2"，插件全局类引用为 "Shit::Vector2"
QString typeNameOf(const Shit::FieldInfo &field)
{
    const std::string &t = field.typeName;
    const std::string prefix = "Shit::";
    if (t.rfind(prefix, 0) == 0)
        return QString::fromStdString(t.substr(prefix.size()));
    return QString::fromStdString(t);
}

QString displayNameOf(const Shit::FieldInfo &field)
{
    if (const FieldMeta *m = metaOf(field); m && !m->displayName.empty())
        return QString::fromStdString(m->displayName);
    return QString::fromStdString(field.name);
}

/// 通用浮点编辑框（Vector2 分量用）
QDoubleSpinBox *makeSpin(float value)
{
    auto *box = new QDoubleSpinBox;
    box->setRange(-1e6, 1e6);
    box->setSingleStep(0.1);
    box->setDecimals(3);
    box->setValue(value);
    return box;
}

} // namespace

Inspector::Inspector(QWidget *parent)
    : QWidget(parent)
    , m_scroll(new QScrollArea(this))
    , m_content(new QWidget)
    , m_form(new QFormLayout(m_content))
{
    m_scroll->setWidget(m_content);
    m_scroll->setWidgetResizable(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scroll);

    m_form->setContentsMargins(8, 8, 8, 8);
    m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    clear();
}

void Inspector::clear()
{
    m_readbacks.clear();
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);

    auto *placeholder = new QLabel(tr("未选中对象\n\n选中场景中的对象后，\n在此编辑其组件属性。"), m_content);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    m_form->addRow(placeholder);
}

void Inspector::refresh()
{
    for (auto &readback : m_readbacks)
        readback();
}

void Inspector::setGameObject(Shit::GameObject *object)
{
    clear();
    m_componentCount = 0;
    m_fieldCount = 0;
    if (!object)
        return;

    // 组件标题之后继续添加行，故移除占位
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);

    object->forEachComponent([this](Shit::Component *component) {
        if (!component) return;

        const Shit::TypeInfo *typeInfo = Shit::TypeRegistry::Get(std::type_index(typeid(*component)));
        if (!typeInfo || typeInfo->fields.empty())
            return; // 无反射元数据（如编辑器自定义 Behavior），跳过

        ++m_componentCount;
        auto *title = new QLabel(QString::fromStdString(typeInfo->name), m_content);
        title->setStyleSheet("font-weight:bold; color:#2a7ab1; margin-top:6px;");
        m_form->addRow(title);

        for (const Shit::FieldInfo &field : typeInfo->fields) {
            addFieldRow(field, component);
            ++m_fieldCount;
        }
    });

    emit buildInfo(m_componentCount, m_fieldCount);
}

void Inspector::addFieldRow(const Shit::FieldInfo &field, Shit::Component *obj)
{
    const bool readOnly = metaOf(field) && metaOf(field)->readOnly;
    const QString name = displayNameOf(field);

    if (readOnly) {
        m_form->addRow(name, new QLabel(fieldToString(field, obj), m_content));
        return;
    }

    if (typeNameOf(field) == "float") {
        auto *box = new QDoubleSpinBox(m_content);
        const FieldMeta *m = metaOf(field);
        if (m && m->range.min != m->range.max)
            box->setRange(m->range.min, m->range.max);
        else
            box->setRange(-1e6, 1e6);
        box->setSingleStep(m && m->step > 0.0f ? m->step : 0.1f);
        box->setDecimals(3);
        box->setValue(*reinterpret_cast<float *>(field.GetFieldPtr(obj)));
        connect(box, &QDoubleSpinBox::valueChanged, this, [this, obj, field](double v) {
            *reinterpret_cast<float *>(field.GetFieldPtr(obj)) = static_cast<float>(v);
            obj->onFieldChanged(field.name);
            emit fieldEdited();
        });
        connect(box, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, box);
        m_readbacks.push_back([box, obj, field] {
            box->blockSignals(true);
            box->setValue(*reinterpret_cast<float *>(field.GetFieldPtr(obj)));
            box->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "int") {
        auto *box = new QSpinBox(m_content);
        box->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        box->setValue(*reinterpret_cast<int *>(field.GetFieldPtr(obj)));
        connect(box, &QSpinBox::valueChanged, this, [this, obj, field](int v) {
            *reinterpret_cast<int *>(field.GetFieldPtr(obj)) = v;
            obj->onFieldChanged(field.name);
            emit fieldEdited();
        });
        connect(box, &QSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, box);
        m_readbacks.push_back([box, obj, field] {
            box->blockSignals(true);
            box->setValue(*reinterpret_cast<int *>(field.GetFieldPtr(obj)));
            box->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "bool") {
        auto *check = new QCheckBox(m_content);
        check->setChecked(*reinterpret_cast<bool *>(field.GetFieldPtr(obj)));
        connect(check, &QCheckBox::toggled, this, [this, obj, field](bool on) {
            *reinterpret_cast<bool *>(field.GetFieldPtr(obj)) = on;
            obj->onFieldChanged(field.name);
            emit fieldEdited();
            emit fieldCommitted();
        });
        m_form->addRow(name, check);
        m_readbacks.push_back([check, obj, field] {
            check->blockSignals(true);
            check->setChecked(*reinterpret_cast<bool *>(field.GetFieldPtr(obj)));
            check->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "Vector2") {
        auto *p = reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj));
        auto *xBox = makeSpin(p->x);
        auto *yBox = makeSpin(p->y);
        connect(xBox, &QDoubleSpinBox::valueChanged, this, [this, obj, field](double v) {
            reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj))->x = static_cast<float>(v);
            obj->onFieldChanged(field.name);
            emit fieldEdited();
        });
        connect(yBox, &QDoubleSpinBox::valueChanged, this, [this, obj, field](double v) {
            reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj))->y = static_cast<float>(v);
            obj->onFieldChanged(field.name);
            emit fieldEdited();
        });
        connect(xBox, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        connect(yBox, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        auto *row = new QWidget(m_content);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(xBox);
        layout->addWidget(yBox);
        m_form->addRow(name, row);
        m_readbacks.push_back([xBox, yBox, obj, field] {
            auto *p = reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj));
            xBox->blockSignals(true);
            yBox->blockSignals(true);
            xBox->setValue(p->x);
            yBox->setValue(p->y);
            xBox->blockSignals(false);
            yBox->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "std::string") {
        auto *p = reinterpret_cast<std::string *>(field.GetFieldPtr(obj));
        auto *edit = new QLineEdit(QString::fromStdString(*p), m_content);
        connect(edit, &QLineEdit::textChanged, this, [this, obj, field](const QString &text) {
            *reinterpret_cast<std::string *>(field.GetFieldPtr(obj)) = text.toStdString();
            obj->onFieldChanged(field.name);
            emit fieldEdited();
        });
        connect(edit, &QLineEdit::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, edit);
        m_readbacks.push_back([edit, obj, field] {
            edit->blockSignals(true);
            edit->setText(QString::fromStdString(*reinterpret_cast<std::string *>(field.GetFieldPtr(obj))));
            edit->blockSignals(false);
        });
    }
    else {
        // 枚举或未知类型：优先尝试枚举下拉，否则只读展示
        const Shit::TypeInfo *enumType = Shit::TypeRegistry::Get(typeNameOf(field).toStdString());
        if (enumType && !enumType->enumValues.empty()) {
            auto *combo = new QComboBox(m_content);
            const int cur = *reinterpret_cast<int *>(field.GetFieldPtr(obj));
            int sel = 0;
            for (size_t i = 0; i < enumType->enumValues.size(); ++i) {
                combo->addItem(QString::fromStdString(enumType->enumValues[i].name),
                               static_cast<int>(enumType->enumValues[i].value));
                if (enumType->enumValues[i].value == cur) sel = static_cast<int>(i);
            }
            combo->setCurrentIndex(sel);
            connect(combo, &QComboBox::currentIndexChanged, this, [this, obj, field, combo]() {
                *reinterpret_cast<int *>(field.GetFieldPtr(obj)) = combo->currentData().toInt();
                obj->onFieldChanged(field.name);
                emit fieldEdited();
                emit fieldCommitted();
            });
            m_form->addRow(name, combo);
            m_readbacks.push_back([combo, obj, field] {
                const int v = *reinterpret_cast<int *>(field.GetFieldPtr(obj));
                const int idx = combo->findData(v);
                combo->blockSignals(true);
                combo->setCurrentIndex(idx >= 0 ? idx : 0);
                combo->blockSignals(false);
            });
        } else {
            m_form->addRow(name, new QLabel(fieldToString(field, obj), m_content));
        }
    }
}

QString Inspector::fieldToString(const Shit::FieldInfo &field, Shit::Component *obj) const
{
    void *ptr = field.GetFieldPtr(obj);
    if (typeNameOf(field) == "float")
        return QString::number(*reinterpret_cast<float *>(ptr), 'g', 4);
    if (typeNameOf(field) == "int")
        return QString::number(*reinterpret_cast<int *>(ptr));
    if (typeNameOf(field) == "bool")
        return *reinterpret_cast<bool *>(ptr) ? tr("是") : tr("否");
    if (typeNameOf(field) == "Vector2") {
        auto *v = reinterpret_cast<Shit::Vector2 *>(ptr);
        return QString("(%1, %2)").arg(v->x).arg(v->y);
    }
    if (typeNameOf(field) == "std::string")
        return QString::fromStdString(*reinterpret_cast<std::string *>(ptr));
    return QString("<%1>").arg(typeNameOf(field));
}