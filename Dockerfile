FROM gcc:latest

WORKDIR /app

COPY . .

RUN gcc -o student_server main.c student.c mongoose.c -lm

EXPOSE 10000

CMD ["./student_server"]
