#include <stdio.h>
#include "string.h"
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/socket.h>
#define I2C_DEV "/dev/i2c-1" // Шлях до пристрою I2C

#define QMC5883L_ADDR 0x0D // Адреса датчика QMC5883L на шині I2C
#define PORT 14555
#define DEST_IP "192.168.88.64"
typedef struct __attribute__((packed)) {
    uint8_t header;
    uint8_t len;
    uint8_t seq;
    uint8_t sysid;
    uint8_t compid;
    uint8_t msgid;
    uint8_t type;
    uint8_t autopilot;
    uint8_t base_mode;
    float custom_mode;
    uint8_t system_status;
    uint8_t mavlink_version;
} mavlink_packet_t;

void send_mavlink_packet(float axis_x,const char* dest_ip) {
    int sockfd;
    struct sockaddr_in dest_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Формування Mavlink пакету
    mavlink_packet_t mavlink_packet;
    mavlink_packet.header = 0xFE;
    mavlink_packet.len = sizeof(mavlink_packet);
    mavlink_packet.seq = 0;
    mavlink_packet.sysid = 1;
    mavlink_packet.compid = 1;
    mavlink_packet.msgid = 0;
    mavlink_packet.type = 6;
    mavlink_packet.autopilot = 0;
    mavlink_packet.base_mode = 0;
    mavlink_packet.custom_mode = axis_x;
    mavlink_packet.system_status = 0;
    mavlink_packet.mavlink_version = 3;

    // Відправка пакету
    sendto(sockfd, &mavlink_packet, sizeof(mavlink_packet), 0, (const struct sockaddr *) &dest_addr, sizeof(dest_addr));
    std::cout<<mavlink_packet.custom_mode; 

    close(sockfd);
}

int main() {
    std::string dest_ip;
    std::cout<<"Enter ip: ";

    std::cin>>dest_ip;
    const char* input = dest_ip.c_str();
    int file;
    char filename[20];
    int x, y, z;

    // Відкриття з'єднання з шиною I2C
    if ((file = open(I2C_DEV, O_RDWR)) < 0) {
        std::cerr << "Failed to open the i2c bus" << std::endl;
        return 1;
    }

    // Встановлення адреси датчика QMC5883L
    if (ioctl(file, I2C_SLAVE, QMC5883L_ADDR) < 0) {
        std::cerr << "Failed to acquire bus access and/or talk to slave" << std::endl;
        return 1;
    }

    // Налаштування режиму роботи датчика
    char buf[2] = {0};
    buf[0] = 0x09; // Адрес регістру CTRL_REG
    buf[1] = 0x01; // Нове значення для встановлення одного заміру
    if (write(file, buf, 2) != 2) {
        std::cerr << "Error writing to QMC5883L" << std::endl;
        return 1;
    }

    // Читання значень магнітного поля
    char data[6];
    buf[0] = 0x00; // Початковий адреси регістру DATA_REG
    if (write(file, buf, 1) != 1) {
        std::cerr << "Error writing to QMC5883L" << std::endl;
        return 1;
    }
    if (read(file, data, 6) != 6) {
        std::cerr << "Error reading from QMC5883L" << std::endl;
        return 1;
    }

    // Обробка отриманих значень магнітного поля
    x = (data[1] << 8) | data[0];
  float axis_x = x;
   
    send_mavlink_packet(axis_x,input);
    return 0;
}
