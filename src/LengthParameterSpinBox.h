#pragma once

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QRegularExpression>
#include <functional>
#include <optional>

class LengthParameterSpinBox final:public QDoubleSpinBox {
public:
    explicit LengthParameterSpinBox(QWidget* parent=nullptr):QDoubleSpinBox(parent){connect(lineEdit(),&QLineEdit::textEdited,this,[this](const QString& input){
        const auto name=namePart(input);const bool identifier=!name.isEmpty() && (name.front().isLetter() || name.front()=='_');
        editedIdentifier_=identifier ? name : QString{};editedLiteral_=!identifier;
        typedName_=identifier && namedValue && namedValue(name).has_value() ? name : QString{};
    });}
    std::function<std::optional<double>(const QString&)> namedValue;
    QString activeName() const {return typedName_;}
    QString enteredIdentifier() const {return editedIdentifier_;}
    bool literalEdited() const {return editedLiteral_;}
    void showBinding(const QString& name){boundName_=name;typedName_=name;editedIdentifier_.clear();editedLiteral_=false;
        lineEdit()->setText(name.isEmpty() ? QString::number(value(),'f',decimals())+suffix() : name);}
protected:
    void stepBy(int steps) override {boundName_.clear();typedName_.clear();editedIdentifier_.clear();editedLiteral_=true;QDoubleSpinBox::stepBy(steps);}
    QValidator::State validate(QString& input,int& cursor) const override {
        const auto name=namePart(input);
        if(!name.isEmpty() && (name.front().isLetter() || name.front()=='_')) {
            static const QRegularExpression identifier("^[A-Za-z_][A-Za-z0-9_]*$");
            if(!identifier.match(name).hasMatch())return QValidator::Invalid;
            return namedValue && namedValue(name).has_value() ? QValidator::Acceptable : QValidator::Intermediate;
        }
        return QDoubleSpinBox::validate(input,cursor);
    }
    double valueFromText(const QString& input) const override {
        if(namedValue)if(const auto named=namedValue(namePart(input)))return *named;
        return QDoubleSpinBox::valueFromText(input);
    }
    QString textFromValue(double value) const override {return boundName_.isEmpty() ? QDoubleSpinBox::textFromValue(value) : boundName_;}
private:
    QString namePart(QString text) const {if(!suffix().isEmpty() && text.endsWith(suffix()))text.chop(suffix().size());return text.trimmed();}
    QString boundName_;
    QString typedName_;
    QString editedIdentifier_;
    bool editedLiteral_=false;
};
