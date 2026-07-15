import psycopg2


s = ' AND 1 = 1'
q1 = f'SELECT * FROM posts WHERE 1 = 1 {s * 100_000};'
q2 = f'SELECT * FROM posts WHERE 1 = 1 {s * 10_000};'
q3 = f'SELECT * FROM posts WHERE 1 = 1 {s * 1_000};'
# print(q1)

url = 'postgresql://postgres:1234@localhost:8888/mydb'
conn = psycopg2.connect(url)
cur = conn.cursor()

cur.execute(q1)
cur.execute(q2)
cur.execute(q3)
