
FROM gcc:latest

WORKDIR /app

# Install PostgreSQL development library
RUN apt-get update && apt-get install -y \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy project files
COPY . .

# Compile C backend with PostgreSQL support
RUN gcc \
    -I/usr/include/postgresql \
    -o student_server \
    main.c \
    student.c \
    mongoose.c \
    -lm \
    -lpq

EXPOSE 10000

CMD ["./student_server"]
