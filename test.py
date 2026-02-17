from nucleus_driver import NucleusDriver

TCP_HOST = '192.168.2.201'  # Using IP adress

driver = NucleusDriver()
driver.set_tcp_configuration(host=TCP_HOST)
driver.connect(connection_type='tcp')
result = driver.start_measurement()

for _ in range(100):

    packet = driver.read_packet(timeout=1, _suppress_warning=True)
    if packet is None:
        print("No packet received within timeout.")
    print(packet["id"])
driver.stop()

driver.disconnect()
