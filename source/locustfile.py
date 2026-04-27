from locust import task, constant, HttpUser, between

class MyWebServerUser(HttpUser):
    wait_time = between(0.1, 1)

    @task(2)
    def index_page(self):
        self.client.get("/")

    @task(1)
    def page_two(self):
        self.client.get("/page2.html")
        
    @task(1)
    def not_found_page(self):
        with self.client.get("/something", catch_response=True) as response:
            if response.status_code == 404:
                response.success()
            elif response.status_code == 0:
                response.failure("No answer")