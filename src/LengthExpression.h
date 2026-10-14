#pragma once
#include <QString>
#include <QRegularExpression>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>
#include <utility>

struct LengthQuantity { double value=0; bool length=false; };
struct LengthIdentifier { QString name; int start=0; int end=0; };
class LengthExpression {
public:
    using Resolver=std::function<LengthQuantity(const QString&,int)>;
    LengthExpression(QString source,Resolver resolver,double contextualUnitMm=0)
        :source_(std::move(source)),resolver_(std::move(resolver)),contextualUnitMm_(contextualUnitMm) {}
    LengthQuantity parse() {
        if(source_.size()>4096)fail("Length expression exceeds 4096 characters");
        const auto result=sum();space();
        if(position_!=source_.size())fail("Unexpected expression token");
        if(!std::isfinite(result.value))fail("Nonfinite length expression");
        return result;
    }
    const std::vector<LengthIdentifier>& identifiers() const {return identifiers_;}
private:
    [[noreturn]] static void fail(const char* message) {throw std::invalid_argument(message);}
    void space(){while(position_<source_.size() && source_[position_].isSpace())++position_;}
    bool take(QChar c){space();if(position_<source_.size() && source_[position_]==c){++position_;return true;}return false;}
    LengthQuantity sum(){
        auto left=product();
        while(true){const bool add=take('+');const bool subtract=!add && take('-');if(!add && !subtract)break;
            auto right=product();
            if(left.length!=right.length && contextualUnitMm_>0){
                auto& scalar=left.length ? right : left;
                scalar.value*=contextualUnitMm_;scalar.length=true;
            }
            if(left.length!=right.length)fail("Addition needs matching length units");
            left.value+=add ? right.value : -right.value;
        }return left;
    }
    LengthQuantity product(){
        auto left=unary();
        while(true){const bool multiply=take('*');const bool divide=!multiply && take('/');if(!multiply && !divide)break;
            const auto right=unary();
            if(multiply){if(left.length && right.length)fail("Length multiplied by length is unsupported");
                left={left.value*right.value,left.length || right.length};
            }else{if(right.length)fail("Dividing by a length is unsupported");
                if(right.value==0)fail("Division by zero");left.value/=right.value;
            }
            if(!std::isfinite(left.value))fail("Nonfinite length expression");
        }return left;
    }
    LengthQuantity unary(){
        struct Guard {int& depth;explicit Guard(int& value):depth(value){if(++depth>64)throw std::invalid_argument("Length expression is too deeply nested");}~Guard(){--depth;}} guard(depth_);
        if(take('+'))return unary();if(take('-')){auto result=unary();result.value=-result.value;return result;}return primary();
    }
    LengthQuantity primary(){
        space();if(take('(')){const auto result=sum();if(!take(')'))fail("Missing closing parenthesis");return result;}
        if(position_>=source_.size())fail("Incomplete length expression");
        if(source_[position_].isLetter() || source_[position_]=='_'){
            const int start=position_++;while(position_<source_.size() && (source_[position_].isLetterOrNumber() || source_[position_]=='_'))++position_;
            const auto name=source_.mid(start,position_-start);const int index=static_cast<int>(identifiers_.size());
            identifiers_.push_back({name,start,position_});return resolver_(name,index);
        }
        const auto tail=source_.mid(position_);
        static const QRegularExpression number(QStringLiteral(R"(^(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)"));
        const auto match=number.match(tail);if(!match.hasMatch())fail("Expected number, name or parenthesis");
        bool ok=false;const double value=match.captured().toDouble(&ok);if(!ok || !std::isfinite(value))fail("Nonfinite length expression");
        position_+=match.capturedLength();space();
        if(position_<source_.size() && source_[position_].isLetter()){
            const int start=position_;while(position_<source_.size() && source_[position_].isLetter())++position_;
            const auto unit=source_.mid(start,position_-start).toLower();
            if(unit!="mm" && unit!="in")fail("Unknown length unit");
            return {value*(unit=="in" ? 25.4 : 1),true};
        }
        return {value,false};
    }
    QString source_;Resolver resolver_;double contextualUnitMm_=0;
    int position_=0,depth_=0;std::vector<LengthIdentifier> identifiers_;
};
