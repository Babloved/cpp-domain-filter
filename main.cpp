#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace std;

class Domain{
public:
    using TypeData = vector<string>; // Тип данных для хранения поддоменов
    using ConstReverseIt = TypeData::const_reverse_iterator;
    // Итератор для обратного обхода

    Domain(string_view raw_domain){
        size_t start = 0;
        size_t end = raw_domain.find('.');
        while (end != string_view::npos){
            // Добавляем поддомен в вектор data_
            data_.push_back(string(raw_domain.substr(start, end - start)));
            start = end + 1;
            end = raw_domain.find('.', start);
        }
        // Добавляем последний поддомен
        data_.push_back(string(raw_domain.substr(start)));
    }

    // Возвращает список поддоменов
    const TypeData& GetSubDomains() const{ return data_; }

    bool operator==(const Domain& other) const{
        if (data_.size() != other.data_.size()){
            return false; // Домены разной длины не могут быть равны
        }
        // Сравниваем поддомены в обратном порядке
        return DomainDataCompare(this->data_.rbegin(), this->data_.rend(),
                                 other.data_.rbegin(), other.data_.rend());
    }

    // Проверяет, является ли текущий домен поддоменом другого домена
    bool IsSubdomain(const Domain& other) const{
        if (data_.size() >= other.data_.size()){
            return false; // Поддомен не может быть длиннее или равен домену
        }
        // Сравниваем поддомены в обратном порядке
        return DomainDataCompare(this->data_.rbegin(), this->data_.rend(),
                                 other.data_.rbegin(), other.data_.rend());
    }

private:
    // Вспомогательная функция для сравнения поддоменов
    bool static DomainDataCompare(ConstReverseIt it_lhs_begin,
                                  ConstReverseIt it_lhs_end,
                                  ConstReverseIt it_rhs_begin,
                                  ConstReverseIt it_rhs_end){
        for (auto it_this_cur = it_lhs_begin, it_oth_cur = it_rhs_begin;
             it_this_cur != it_lhs_end && it_oth_cur != it_rhs_end;
             ++it_oth_cur, ++it_this_cur){
            if (*it_this_cur != *it_oth_cur){
                return false; // Поддомены не совпадают
            }
        }
        return true; // Все поддомены совпали
    }

    TypeData data_; // Вектор для хранения поддоменов
};

// Структура для представления узла дерева поддоменов
struct SubDomain{
    unordered_map<string_view, unique_ptr<SubDomain>> sub_domains_;
    // Хранит поддомены
};

// Класс для проверки доменов на запрещённость
class DomainChecker{
public:
    // Конструктор, принимающий диапазон доменов и строящий дерево запрещённых
    // доменов
    template <typename It>
    DomainChecker(It it_begin, It it_end) : domains_(it_begin, it_end){
        // Сортируем домены по длине (от коротких к длинным)
        sort(domains_.begin(), domains_.end(),
             [](const auto& lhs, const auto& rhs){
                 return lhs.GetSubDomains().size() < rhs.GetSubDomains().size();
             });

        // Строим дерево запрещённых доменов
        for (const auto& domain : domains_){
            auto top = &forbidden_domains_; // Начинаем с корня дерева

            auto& sub_domains = domain.GetSubDomains(); // Получаем поддомены

            // Обходим поддомены в обратном порядке (от верхнего уровня к нижнему)
            for (auto it_current = sub_domains.rbegin();
                 it_current != sub_domains.rend(); ++it_current){
                auto it_find = top->sub_domains_.find(*it_current);
                // Ищем поддомен в текущем уровне
                if (it_find != top->sub_domains_.end() && it_find->second == nullptr){
                    break; // Если поддомен уже помечен как запрещённый, выходим
                }

                auto& child = top->sub_domains_[*it_current];
                // Создаём или получаем дочерний узел
                if (&*it_current != &sub_domains.front()){
                    if (!child){
                        child = make_unique<SubDomain>();
                        // Создаём новый узел, если его нет
                    }
                    top = child.get(); // Переходим на следующий уровень
                } else{
                    child.reset(); // Помечаем конец домена (запрещённый домен)
                }
            }
        }
    }

    // Проверяет, является ли домен запрещённым
    bool IsForbidden(const Domain& domain) const{
        const auto& sub_domains = domain.GetSubDomains(); // Получаем поддомены
        auto top = &forbidden_domains_; // Начинаем с корня дерева

        // Обходим поддомены в обратном порядке
        for (auto it_current = sub_domains.rbegin();
             it_current != sub_domains.rend(); ++it_current){
            auto it_find = top->sub_domains_.find(*it_current);
            // Ищем поддомен в текущем уровне
            if (it_find != top->sub_domains_.end()){
                if (top->sub_domains_.at(*it_current) == nullptr){
                    return true; // Найден запрещённый домен
                } else{
                    top = it_find->second.get(); // Переходим на следующий уровень
                }
            } else{
                return false; // Поддомен не найден, домен не запрещён
            }
        }
        return false; // Домен не запрещён
    }

private:
    SubDomain forbidden_domains_; // Корень дерева запрещённых доменов
    vector<Domain> domains_; // Список всех запрещённых доменов
};

// Читает заданное количество доменов из входного потока
vector<Domain> ReadDomains(istream& input, size_t domains_count){
    input >> ws; // Пропускаем пробелы
    string temp_input;
    vector<Domain> result;
    for (size_t i = 0; i < domains_count; i++){
        getline(input, temp_input); // Читаем строку
        result.push_back(Domain(temp_input));
        // Создаём Domain и добавляем в результат
    }
    return result;
}

// Читает число из строки входного потока
template <typename Number>
Number ReadNumberOnLine(istream& input){
    string line;
    getline(input, line); // Читаем строку
    Number num;
    std::istringstream(line) >> num; // Преобразуем строку в число

    return num;
}

int main(){
    const std::vector<Domain> forbidden_domains =
        ReadDomains(cin, ReadNumberOnLine<size_t>(cin));
    DomainChecker checker(forbidden_domains.begin(), forbidden_domains.end());
    const std::vector<Domain> test_domains =
        ReadDomains(cin, ReadNumberOnLine<size_t>(cin));
    for (const Domain& domain : test_domains){
        cout << (checker.IsForbidden(domain) ? "Bad"sv : "Good"sv) << endl;
    }
    return 0;
}