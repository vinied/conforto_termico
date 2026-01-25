#ifndef DHT11_ACCESS_H_
#define DHT11_ACCESS_H_

void DHT11_run(TASKS_t running_task);
float Dht11GetHumidity(void);
float Dht11GetTemperature(void);

#endif /* DHT11_ACCESS_H_ */
