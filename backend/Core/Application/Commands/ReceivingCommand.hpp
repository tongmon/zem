#ifndef CORE_APPLICATION_COMMANDS_RECEIVINGCOMMAND_HPP
#define CORE_APPLICATION_COMMANDS_RECEIVINGCOMMAND_HPP

#include <string>

// This is a example struct
struct ReceivingCommand
{
    std::string warehouse_id;
    std::string sku;
    int qty = 0;
    std::string inbound_id;
};

#endif /* CORE_APPLICATION_COMMANDS_RECEIVINGCOMMAND_HPP */
