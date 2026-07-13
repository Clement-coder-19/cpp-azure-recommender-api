#include "crow.h" 
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace std;

class User {
public:
    vector<pair<int,double>> ratings;
    int id;
    double norm;
    User(int id):id(id),norm(0.0) {}
    void addRating(int item, double rating) { ratings.emplace_back(item,rating); }
    void sortRatings() { sort(ratings.begin(),ratings.end()); }
    void calculateNorm() {
        norm=0.0;
        for (auto &r:ratings) { norm+=r.second*r.second; }
        norm=sqrt(norm);
    }
    bool hasItem(int item) {
        for (auto &r:ratings) { if (r.first==item) return true; }
        return false;
    }
};

double cosineSimilarity(User &u1, User &u2) {
    int i=0, j=0;
    double dot_product=0.0;
    while(i<u1.ratings.size() && j<u2.ratings.size()) {
        if (u1.ratings[i].first==u2.ratings[j].first) {
            dot_product+=u1.ratings[i].second*u2.ratings[j].second;
            i++; j++;
        }
        else if (u1.ratings[i].first<u2.ratings[j].first) { i++; }
        else { j++; }
    }
    if (u1.norm==0 || u2.norm==0) return 0.0;
    return dot_product/(u1.norm*u2.norm);
}

double predictRating(User &u, int item, vector<User> &users, unordered_map<int,double> &similarities) {
    double numerator=0.0;
    double denominator=0.0;
    for (auto &other:users) {
        if (other.id==u.id) continue;
        bool found=false;
        double rating=0.0;
        for (auto &r: other.ratings) {
            if (r.first==item) { found=true; rating=r.second; break; }
        }
        if (!found) continue;
        double users_similarities=similarities[other.id];
        numerator+=users_similarities*rating;
        denominator+=abs(users_similarities);
    }
    if (denominator==0) return 0.0;
    return numerator/denominator;
}

vector<pair<int, double>> recommend(int userId, int N, vector<User>& users) {
    User *user=nullptr;
    for (auto &u:users) {
        if(u.id==userId) { user=&u; break; }
    }
    if (!user) return {};
    
    unordered_map<int,double> similarities;
    for (auto &other:users) {
        if (other.id!=user->id) {
            similarities[other.id]=cosineSimilarity(*user,other);
        }
    }
    unordered_set<int> otherItems;
    for(auto &u:users) {
        for (auto &r:u.ratings) {
            if (!user->hasItem(r.first)) { otherItems.insert(r.first); }
        }
    }
    
    vector<pair<int,double>> recommendations;
    for (int item: otherItems) {
        double score=predictRating(*user,item,users,similarities);
        recommendations.emplace_back(item,score);
    }
    sort(recommendations.begin(), recommendations.end(), [](auto &a, auto &b) {
        return a.second>b.second;
    });
    if (recommendations.size()>N) recommendations.resize(N);
    return recommendations;
}

void loadData(string filename, vector<User> &users) {
    fstream file(filename, ios::in);
    if (!file) {
        cout << "Error when opening the file" << endl;
        return;
    }
    int id_user, item;
    double rating;
    unordered_map<int,int> index_user;
    while(file>>id_user>>item>>rating) {
        if (!index_user.count(id_user)) {
            users.emplace_back(id_user);
            index_user[id_user]=users.size()-1;
        }
        users[index_user[id_user]].addRating(item,rating);
    }
    file.close();
}

int main() {
    crow::SimpleApp app;
    vector<User> users;

   
    loadData("dataset/data.txt", users);
    for (auto &u : users) {
        u.sortRatings();
        u.calculateNorm();
    }

    CROW_ROUTE(app, "/health")([](){
        return crow::response(200, "OK");
    });

    CROW_ROUTE(app, "/recommend")([&users](const crow::request& req){
        int userId = 1;
        int N = 5;

        if (req.url_params.get("userId") != nullptr) {
            userId = stoi(req.url_params.get("userId"));
        }
        if (req.url_params.get("topN") != nullptr) {
            N = stoi(req.url_params.get("topN"));
        }

        auto recs = recommend(userId, N, users);

        crow::json::wvalue response_json;
        response_json["user_id"] = userId;
        
        crow::json::wvalue::list recs_list;
        for (auto &r : recs) {
            crow::json::wvalue item;
            item["item_id"] = r.first;
            item["score"] = r.second;
            recs_list.push_back(item);
        }
        response_json["recommendations"] = move(recs_list);

        return crow::response(200, response_json);
    });

    app.port(8080).multithreaded().run();
}
