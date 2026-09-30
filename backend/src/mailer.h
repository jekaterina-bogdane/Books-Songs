#pragma once
#include <string>

// Send an HTML/text receipt email via curl SMTP.
// Reads SMTP credentials from <data_dir>/smtp.conf (BSF format):
//   url:  smtps://smtp.gmail.com:465
//   user: you@gmail.com
//   pass: your-app-password
//   from: you@gmail.com
//
// If smtp.conf is absent the receipt is printed to stdout and saved to
// <data_dir>/receipts/order_<id>.txt — no email is sent.
bool send_receipt_email(
    const std::string& to_email,
    const std::string& to_name,
    const std::string& order_id,
    const std::string& subject,
    const std::string& body_text,
    const std::string& data_dir
);
