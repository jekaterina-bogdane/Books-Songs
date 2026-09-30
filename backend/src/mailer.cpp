#include "mailer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdio>
#include <map>

static std::map<std::string,std::string> load_smtp_conf(const std::string& data_dir) {
    std::map<std::string,std::string> cfg;
    std::ifstream f(data_dir + "/smtp.conf");
    if (!f.is_open()) return cfg;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto idx = line.find(": ");
        if (idx == std::string::npos) continue;
        cfg[line.substr(0, idx)] = line.substr(idx + 2);
    }
    return cfg;
}

// Escape single quotes for shell argument safety
static std::string sh_esc(std::string s) {
    std::string r; r.reserve(s.size() + 4);
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else r += c;
    }
    return r;
}

bool send_receipt_email(
    const std::string& to_email,
    const std::string& to_name,
    const std::string& order_id,
    const std::string& subject,
    const std::string& body_text,
    const std::string& data_dir
) {
    // Always save receipt to disk
    std::string receipts_dir = data_dir + "/receipts";
    std::filesystem::create_directories(receipts_dir);
    std::string receipt_path = receipts_dir + "/order_" + order_id + ".txt";
    {
        std::ofstream rf(receipt_path);
        rf << body_text;
    }
    std::cout << "[Mailer] Receipt saved: " << receipt_path << "\n";

    auto cfg = load_smtp_conf(data_dir);
    if (cfg["url"].empty() || cfg["user"].empty() || cfg["pass"].empty()) {
        std::cout << "[Mailer] No smtp.conf — email not sent. "
                     "Add data/smtp.conf to enable real email delivery.\n";
        return false;
    }

    // Write RFC 2822 email to a temp file
    std::string tmp = receipts_dir + "/tmp_" + order_id + ".eml";
    {
        std::ofstream t(tmp);
        std::string from_addr = cfg.count("from") ? cfg["from"] : cfg["user"];
        t << "From: Books & Songs <" << from_addr << ">\r\n"
          << "To: " << to_name << " <" << to_email << ">\r\n"
          << "Subject: " << subject << "\r\n"
          << "Content-Type: text/plain; charset=utf-8\r\n"
          << "MIME-Version: 1.0\r\n"
          << "\r\n"
          << body_text;
    }

    std::string cmd =
        "curl -s"
        " --url '"        + sh_esc(cfg["url"])  + "'"
        " --ssl-reqd"
        " --mail-from '"  + sh_esc(cfg.count("from") ? cfg["from"] : cfg["user"]) + "'"
        " --mail-rcpt '"  + sh_esc(to_email)    + "'"
        " --user '"       + sh_esc(cfg["user"]) + ":" + sh_esc(cfg["pass"]) + "'"
        " --upload-file '" + sh_esc(tmp) + "'"
        " 2>&1";

    std::cout << "[Mailer] Sending to " << to_email << " via " << cfg["url"] << "\n";

    FILE* p = popen(cmd.c_str(), "r");
    if (!p) {
        std::cout << "[Mailer] popen failed\n";
        return false;
    }
    char buf[256]; std::string out;
    while (fgets(buf, sizeof(buf), p)) out += buf;
    int rc = pclose(p);

    std::filesystem::remove(tmp);  // clean up temp file

    if (rc == 0) {
        std::cout << "[Mailer] Email delivered to " << to_email << "\n";
        return true;
    }
    std::cout << "[Mailer] curl error (rc=" << rc << "): " << out << "\n";
    return false;
}
