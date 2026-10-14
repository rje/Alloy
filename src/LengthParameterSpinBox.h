#pragma once

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QRegularExpression>
#include <functional>
#include <optional>

class LengthParameterSpinBox final:public QDoubleSpinBox {
public:
    explicit LengthParameterSpinBox(QWidget* parent=nullptr):QDoubleSpinBox(parent){connect(lineEdit(),&QLineEdit::textEdited,this,[this](const QString& input){
        const auto name=namePart(input);const bool formula=allowExpressions && expressionText(name);
        const bool identifier=!formula && !name.isEmpty() && (name.front().isLetter() || name.front()=='_');
        editedExpression_=formula ? name : QString{};editedIdentifier_=identifier ? name : QString{};editedLiteral_=!formula && !identifier;
        typedName_=identifier && namedValue && namedValue(name).has_value() ? name : QString{};
    });}
    std::function<std::optional<double>(const QString&)> namedValue;
    std::function<std::optional<double>(const QString&)> expressionValue;
    bool allowExpressions=false;
    QString activeName() const {return typedName_;}
    QString enteredIdentifier() const {return editedIdentifier_;}
    QString enteredExpression() const {return editedExpression_;}
    bool literalEdited() const {return editedLiteral_;}
    void showBinding(const QString& name){boundName_=name;typedName_=name;editedIdentifier_.clear();editedLiteral_=false;
        editedExpression_.clear();
        lineEdit()->setText(name.isEmpty() ? QString::number(value(),'f',decimals())+suffix() : name);}
    void showExpression(const QString& expression){boundName_.clear();typedName_.clear();editedIdentifier_.clear();editedLiteral_=false;
        editedExpression_.clear();lineEdit()->setText(expression);}
protected:
    void stepBy(int steps) override {boundName_.clear();typedName_.clear();editedIdentifier_.clear();editedExpression_.clear();editedLiteral_=true;QDoubleSpinBox::stepBy(steps);}
    QValidator::State validate(QString& input,int& cursor) const override {
        const auto name=namePart(input);
        if(allowExpressions && expressionText(name))return expressionValue && expressionValue(name).has_value() ? QValidator::Acceptable : QValidator::Intermediate;
        if(!name.isEmpty() && (name.front().isLetter() || name.front()=='_')) {
            static const QRegularExpression identifier("^[A-Za-z_][A-Za-z0-9_]*$");
            if(!identifier.match(name).hasMatch())return QValidator::Invalid;
            return namedValue && namedValue(name).has_value() ? QValidator::Acceptable : QValidator::Intermediate;
        }
        return QDoubleSpinBox::validate(input,cursor);
    }
    double valueFromText(const QString& input) const override {
        if(allowExpressions && expressionText(namePart(input)) && expressionValue)
            if(const auto value=expressionValue(namePart(input)))return *value;
        if(namedValue)if(const auto named=namedValue(namePart(input)))return *named;
        return QDoubleSpinBox::valueFromText(input);
    }
    QString textFromValue(double value) const override {return boundName_.isEmpty() ? QDoubleSpinBox::textFromValue(value) : boundName_;}
private:
    static bool expressionText(const QString& text){
        return text.contains('*') || text.contains('/') || text.contains('(') || text.contains(')')
            || text.indexOf('+',1)>=0 || text.indexOf('-',1)>=0
            || ((text.startsWith('+') || text.startsWith('-')) && text.contains(QRegularExpression("[A-Za-z]")))
            || (text.contains(' ') && text.contains(QRegularExpression("[A-Za-z]")));
    }
    QString namePart(QString text) const {if(!suffix().isEmpty() && text.endsWith(suffix()))text.chop(suffix().size());return text.trimmed();}
    QString boundName_;
    QString typedName_;
    QString editedIdentifier_;
    QString editedExpression_;
    bool editedLiteral_=false;
};
