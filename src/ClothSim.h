#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

#ifdef OSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <glm/glm.hpp>

#define PI 3.14159265
inline float sqr(float x) { return x*x; }

#define DELTA_T 0.005

using namespace std;

class Particle {
  public:
    bool movable;
    float mass;
    glm::vec3 pos;
    glm::vec3 prev_pos;
    glm::vec3 vel;
    glm::vec3 acceleration;
    glm::vec3 normal;
    glm::vec3 color;

    Particle(glm::vec3 p) {
        pos = p; 
        prev_pos = p;

        movable = true;
        mass = 1.0f;
        vel = glm::vec3(0.0f, 0.0f, 0.0f);
        acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
        normal = glm::vec3(0.0f, 0.0f, 0.0f);
        color = glm::vec3();
    }

    Particle() {}

    void addForce(glm::vec3 force) {
        acceleration += (force/mass);
    }

    void addNormal(glm::vec3 n) {
        normal += glm::normalize(n);
    }

    void resetNormal() {
        normal = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    void move(glm::vec3 dir) {
        pos = pos + dir;
    }

    void timeStep() {
        if (movable) {
            //Verlet Intergration.
            glm::vec3 temp = pos;
            pos += (pos-prev_pos) + acceleration*(float)DELTA_T;
            prev_pos = temp;
            
            vel = pos-prev_pos;

            //vel += acceleration * (float) DELTA_T;
            
            //Euler Integration.
            //vel += acceleration * (float) DELTA_T;
            //pos += vel * (float) DELTA_T;

            color = glm::vec3((acceleration.x)*15.0f, (acceleration.y)*15.0f, (acceleration.z)*15.0f);
            acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }
};

class SpringDamper {
  public:
    Particle *p1, *p2;
    float restLength;
    float ks, kd;

    SpringDamper(Particle *pp1, Particle *pp2, float s, float d) {
        ks = s; kd = d;
        p1 = pp1; p2 = pp2;
        restLength = glm::distance(p1->pos, p2->pos);
    }

    SpringDamper() {}

    void timeStep() {
        float dist = glm::distance(p1->pos, p2->pos);
        float distDiff = dist - restLength;
        float damper = glm::dot(p1->vel-p2->vel, p1->pos-p2->pos) / dist;

        glm::vec3 f1 = (p1->pos - p2->pos) * (0.0f - (ks*distDiff + kd*damper) / dist);
        glm::vec3 f2 = -f1;
        p1->addForce(f1); p2->addForce(f2);
    }
};

class Cloth {
  private:
    std::vector<Particle> particlesBACKUP;
    std::vector<SpringDamper> spsBACKUP;
  public:
    int widthNum, heightNum;
    std::vector<Particle> particles;
    std::vector<SpringDamper> sps;
    float ks, kd;
    bool isSoft;

    // For aerodynamic.
    glm::vec3 windVel;

    // For ball collision.
    bool ballCol;
    glm::vec3 center;
    float radius;

    // For cube collision.
    bool cubeCol;
    glm::vec3 cubeMax, cubeMin;

    // For floor collision.
    bool hasFloor;
    float floor;

    Cloth() {}

    void makeBendingSpringDamper(Particle *p1, Particle *p2, bool isSoft) {
        if (isSoft) {
            sps.push_back(SpringDamper(p1, p2, ks*0.8f, kd*0.8f));
        } else {
            sps.push_back(SpringDamper(p1, p2, ks, kd));
        }   
    }

    void makeSpringDamper (Particle *p1, Particle *p2) {
        sps.push_back(SpringDamper(p1, p2, ks, kd));
    }

    Particle* getParticle(int x, int y) {
        return &(particles[y*widthNum + x]);
    }

    void setMovable(int x, int y, bool m) {
        getParticle(x, y)->movable = m; 
    }

    void drop() {
        for(int x=0; x<widthNum; x++) {
            for(int y=0; y<heightNum; y++) {
                setMovable(x, y, true);
            }
        }
    }
    
    Cloth(float width, float height, int wn, int hn, float s, float d, bool soft) {
        ks = s; kd = d;
        widthNum = wn; heightNum = hn;
        particles.resize(widthNum*heightNum);
        isSoft = soft;
        windVel = glm::vec3();
        hasFloor = false;

        // Initialize ball collision information.
        ballCol = false;
        center = glm::vec3();
        radius = 0.0f;

        // Initialize cube collision information.
        cubeCol = false;
        cubeMin = glm::vec3(); cubeMax = glm::vec3();

        // Creating Particles.
        for(int x=0; x<widthNum; x++) {
            for(int y=0; y<heightNum; y++) {
                glm::vec3 pos = glm::vec3(width * (x/(float)widthNum),
                                -height * (y/(float)heightNum),
                                0);
                particles[y*widthNum+x]= Particle(pos);
            }
        }

        // Creating Spring Dampers.
        for(int x=0; x<widthNum; x++) {
            for(int y=0; y<heightNum; y++) {
                if (x<widthNum-1) makeSpringDamper(getParticle(x,y),getParticle(x+1,y));
                if (y<heightNum-1) makeSpringDamper(getParticle(x,y),getParticle(x,y+1));
                if (x<widthNum-1 && y<heightNum-1) makeSpringDamper(getParticle(x,y),getParticle(x+1,y+1));
                if (x<widthNum-1 && y<heightNum-1) makeSpringDamper(getParticle(x+1,y),getParticle(x,y+1));
            }
        }

        for(int x=0; x<widthNum; x++) {
            for(int y=0; y<heightNum; y++) {
                if (x<widthNum-2) makeBendingSpringDamper(getParticle(x,y),getParticle(x+2,y),isSoft);
                if (y<heightNum-2) makeBendingSpringDamper(getParticle(x,y),getParticle(x,y+2),isSoft);
                if (x<widthNum-2 && y<heightNum-2) makeBendingSpringDamper(getParticle(x,y),getParticle(x+2,y+2),isSoft);
                if (x<widthNum-2 && y<heightNum-2) makeBendingSpringDamper(getParticle(x+2,y),getParticle(x,y+2),isSoft);
            }
        }

        // Making unmovable points.
        for(int i=0;i<3; i++)
        {
            getParticle(0+i ,0)->movable = false; 
            getParticle(widthNum-1-i ,0)->movable = false;
            getParticle(widthNum/2-3+i, 0)->movable = false;
        }

        // Save backups.
        particlesBACKUP = particles;
        spsBACKUP = sps;
    }

    Cloth(std::vector<Particle> p, int wn, int hn, float s, float d, bool soft) {
        ks = s; kd = d;
        widthNum = wn; heightNum = hn;
        particles = p;
        isSoft = soft;
        windVel = glm::vec3();
        hasFloor = false;

        // Initialize ball collision information.
        ballCol = false;
        center = glm::vec3();
        radius = 0.0f;

        // Initialize cube collision information.
        cubeCol = false;
        cubeMin = glm::vec3(); cubeMax = glm::vec3();

        // Creating Spring Dampers.
        for(int x=0; x<widthNum; x++) {
            for(int y=0; y<heightNum; y++) {
                if (x<widthNum-1) makeSpringDamper(getParticle(x,y),getParticle(x+1,y));
                if (y<heightNum-1) makeSpringDamper(getParticle(x,y),getParticle(x,y+1));
                if (x<widthNum-1 && y<heightNum-1) makeSpringDamper(getParticle(x,y),getParticle(x+1,y+1));
                if (x<widthNum-1 && y<heightNum-1) makeSpringDamper(getParticle(x+1,y),getParticle(x,y+1));
            }
        }

        for(int x=0; x<widthNum; x++) {
            for(int y=0; y<heightNum; y++) {
                if (x<widthNum-2) makeBendingSpringDamper(getParticle(x,y),getParticle(x+2,y),isSoft);
                if (y<heightNum-2) makeBendingSpringDamper(getParticle(x,y),getParticle(x,y+2),isSoft);
                if (x<widthNum-2 && y<heightNum-2) makeBendingSpringDamper(getParticle(x,y),getParticle(x+2,y+2),isSoft);
                if (x<widthNum-2 && y<heightNum-2) makeBendingSpringDamper(getParticle(x+2,y),getParticle(x,y+2),isSoft);
            }
        }

        // Save backups.
        particlesBACKUP = particles;
        spsBACKUP = sps;
    }

    // For intersection tests.
    bool triangleIntersect(glm::vec3 old, glm::vec3 cur, float& intersect, glm::vec3 A, glm::vec3 B, glm::vec3 C) {
        glm::vec3 D = cur - old;
        float a, b, c, d, e, f, g, h, i, j, k, l, M, t, gamma, beta;
        float tMin = 0.0f, tMax = 1.0f;

        a = A.x - B.x;
        b = A.y - B.y;
        c = A.z - B.z;
        d = A.x - C.x;
        e = A.y - C.y;
        f = A.z - C.z;
        g = D.x;
        h = D.y;
        i = D.z;
        j = A.x - old.x;
        k = A.y - old.y;
        l = A.z - old.z;
        M = a*(e*i - h*f) + b*(g*f - d*i) + c*(d*h - e*g);

        t = (f*(a*k - j*b) + e*(j*c - a*l) + d*(b*l - k*c))/(-M);
        if (t < tMin or t > tMax) {
            return false;
        }

        gamma = (i*(a*k - j*b) + h*(j*c - a*l) + g*(b*l - k*c))/M;
        if (gamma < 0 or gamma > 1) {
            return false;
        }
        beta = (j*(e*i - h*f) + k*(g*f - d*i) + l*(d*h - e*g))/M;
        if (beta < 0 or beta > (1-gamma)) {
            return false;
        }

        // Check NaN.
        if (t != t) {
            t = 0.0f;
        }

        intersect = t;
        return true;
    }

    bool ballIntersect(glm::vec3 old, glm::vec3 cur, float& intersect) {
        glm::vec3 dir = cur - old;

        glm::vec3 eMinusC = old - center;
        float B_2 = sqr(glm::dot(dir, eMinusC));
        float AC_4 = glm::dot(dir, dir) * (glm::dot(eMinusC, eMinusC) - sqr(radius));
        float dis = B_2-AC_4;

        // Tangent and cross are both intersect.
        if (dis < 0) { return false; }

        float minusB = -glm::dot(dir, eMinusC);
        float D_2 = glm::dot(dir, dir);

        float t;
        t = (minusB-sqrt(dis)) / D_2;
        
        // Check for Nan.
        if (t != t) {t = 0.0f;}

        if (!(0.0f <= t and t <= 1.0f)) { return false; }

        intersect = t;

        return true;
    }

    bool hitSurface(glm::vec3 old, glm::vec3 cur, float& intersect, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4) {
        float v123Intersect, v134Intersect;

        bool hit123 = triangleIntersect(old, cur, v123Intersect, v1, v2, v3);
        bool hit134 = triangleIntersect(old, cur, v134Intersect, v1, v3, v4);

        if (hit123) {
            intersect = v123Intersect;
            return hit123;
        } else if (hit134) {
            intersect = v134Intersect;
            return hit134;
        } else {
            return false;
        }
    }

    // For floor.
    bool hitFloor(glm::vec3 old, glm::vec3 cur, float& intersect) {
        glm::vec3 A = glm::vec3(-1000.0f, floor, -1000.0f);
        glm::vec3 B = glm::vec3(-1000.0f, floor, 1000.0f);
        glm::vec3 C = glm::vec3(1000.0f, floor, 1000.0f);
        glm::vec3 D = glm::vec3(1000.0f, floor, -1000.0f);
        return hitSurface(old, cur, intersect, A, B, C, D);
    }

    void setFloor(float f) {
        hasFloor = true;
        floor = f;
    }

    // For gravity.
    void addForce(glm::vec3 force) {
        for(std::vector<Particle>::size_type i = 0; i < particles.size(); i++) {
            particles[i].addForce(force);
        }
    }

    // For aerodynamic.
    glm::vec3 getNormal(Particle *p1,Particle *p2,Particle *p3) {
        glm::vec3 v1 = p2->pos - p1->pos;
        glm::vec3 v2 = p3->pos - p1->pos;

        return glm::normalize(glm::cross(v1, v2));
    }

    float getArea(Particle *p1,Particle *p2,Particle *p3) {
        glm::vec3 v1 = p2->pos - p1->pos;
        glm::vec3 v2 = p3->pos - p1->pos;

        return 0.5f * glm::cross(v1, v2).length();
    }

    void addWindForce(Particle *p1,Particle *p2,Particle *p3) {
        float density = 0.5f;
        float cod = 1.0f;

        glm::vec3 normal = getNormal(p1,p2,p3);

        glm::vec3 vSurface = (p1->vel + p2->vel + p3->vel) / 3.0f;
        glm::vec3 vRelative = vSurface - windVel;

        float a0 = getArea(p1, p2, p3);
        float a = (a0/vRelative.length())* glm::dot(vRelative, normal);

        glm::vec3 force = normal * (-0.5f * density * cod * a * sqr(vRelative.length()));
        p1->addForce(force);
        p2->addForce(force);
        p3->addForce(force);
    }

    void windForce() {
        for(int x = 0; x<widthNum - 1; x++) {
            for(int y=0; y<heightNum - 1; y++) {
                addWindForce(getParticle(x+1,y), getParticle(x,y), getParticle(x,y+1));
                addWindForce(getParticle(x+1,y+1), getParticle(x+1,y), getParticle(x,y+1));
            }
        }
        windVel = glm::vec3();
    }

    void updateWindForce(glm::vec3 wV) {
        windVel = wV;
    }

    // For drawing.
    void drawTriangle(Particle *p1, Particle *p2, Particle *p3, glm::vec3 color) {
        glColor3f(color.x, color.y, color.z);
        
        glm::vec3 p1n = glm::normalize(p1->normal);
        glNormal3f(p1n.x, p1n.y, p1n.z);
        glVertex3f(p1->pos.x, p1->pos.y, p1->pos.z);

        glm::vec3 p2n = glm::normalize(p2->normal);
        glNormal3f(p2n.x, p2n.y, p2n.z);
        glVertex3f(p2->pos.x, p2->pos.y, p2->pos.z);

        glm::vec3 p3n = glm::normalize(p3->normal);
        glNormal3f(p3n.x, p3n.y, p3n.z);
        glVertex3f(p3->pos.x, p3->pos.y, p3->pos.z);
    }

    void draw() {
        for(std::vector<Particle>::size_type i = 0; i < particles.size(); i++) {
            particles[i].resetNormal();
        }

        for(int x=0; x<widthNum - 1; x++) {
            for(int y=0; y<heightNum - 1; y++) {
                glm::vec3 normal = getNormal(getParticle(x+1,y),getParticle(x,y),getParticle(x,y+1));
                getParticle(x+1,y)->addNormal(normal);
                getParticle(x,y)->addNormal(normal);
                getParticle(x,y+1)->addNormal(normal);

                normal = getNormal(getParticle(x+1,y+1),getParticle(x+1,y),getParticle(x,y+1));
                getParticle(x+1,y+1)->addNormal(normal);
                getParticle(x+1,y)->addNormal(normal);
                getParticle(x,y+1)->addNormal(normal);

                normal = getNormal(getParticle(x+1, y+1),getParticle(x+1, y),getParticle(x,y));
                getParticle(x+1,y+1)->addNormal(normal);
                getParticle(x+1,y)->addNormal(normal);
                getParticle(x,y)->addNormal(normal);

                normal = getNormal(getParticle(x+1, y+1),getParticle(x,y),getParticle(x, y+1));
                getParticle(x+1,y+1)->addNormal(normal);
                getParticle(x,y)->addNormal(normal);
                getParticle(x,y+1)->addNormal(normal);
            }
        }
        int p;
        glBegin(GL_TRIANGLES);
        for(int x=0; x<widthNum - 1; x++)
        {
            p = x % 2;
            for(int y=0; y<heightNum - 1; y++)
            {
                glm::vec3 color;
                if ((y+p)%2) // red and white color is interleaved according to which column number
                    color = glm::vec3(1.0f,0.0f,0.0f);
                else
                    color = glm::vec3(1.0f,1.0f,0.0f);

                drawTriangle(getParticle(x+1,y), getParticle(x,y), getParticle(x,y+1), color);
                drawTriangle(getParticle(x+1,y+1), getParticle(x+1,y), getParticle(x,y+1), color);
                drawTriangle(getParticle(x+1,y+1), getParticle(x+1,y), getParticle(x,y), color);
                drawTriangle(getParticle(x+1,y+1), getParticle(x,y), getParticle(x,y+1), color);
            }
        }
        glEnd();
    }

    void drawBalls(bool showForce, float size) {
        glMatrixMode(GL_MODELVIEW);
        glColor3f(1.0f, 1.0f, 1.0f);
        for(std::vector<Particle>::size_type i = 0; i < particles.size(); i++) {
            glPushMatrix();
            glTranslatef(particles[i].pos.x, particles[i].pos.y, particles[i].pos.z);
            if (showForce) {glColor3f(particles[i].color.x, particles[i].color.y, particles[i].color.z);}
            glutSolidSphere(size, 5, 5);
            glPopMatrix();
        }
    }

    // For collisions.
    void withBall(glm::vec3 c, float r) {
        ballCol = true;
        center = c; radius = r;
    }

    void withCube(glm::vec3 in, glm::vec3 ax) {
        cubeCol = true;
        cubeMin = in; cubeMax = ax;
    }

    // For timeStep.
    void timeStep() {

        //Aerodynamic force.
        windForce();

        //Spring Damper force.
        for(std::vector<SpringDamper>::size_type i = 0; i < sps.size(); i++) {
            sps[i].timeStep();
        }

        // Calculate cube info for late use.
        glm::vec3 v1 = glm::vec3(cubeMin.x, cubeMin.y, cubeMin.z);
        glm::vec3 v2 = glm::vec3(cubeMin.x, cubeMin.y, cubeMax.z);
        glm::vec3 v3 = glm::vec3(cubeMax.x, cubeMin.y, cubeMax.z);
        glm::vec3 v4 = glm::vec3(cubeMax.x, cubeMin.y, cubeMin.z);

        glm::vec3 v5 = glm::vec3(cubeMin.x, cubeMax.y, cubeMax.z);
        glm::vec3 v6 = glm::vec3(cubeMin.x, cubeMax.y, cubeMin.z);
        glm::vec3 v7 = glm::vec3(cubeMax.x, cubeMax.y, cubeMin.z);
        glm::vec3 v8 = glm::vec3(cubeMax.x, cubeMax.y, cubeMax.z);

        //Update particles positions with collision detection.
        for(std::vector<Particle>::size_type i = 0; i < particles.size(); i++) {
            glm::vec3 old = particles[i].pos;
            particles[i].timeStep();
            float intersect;

            // Adjust for floor collision.
            if (hasFloor) {
                if (hitFloor(old, particles[i].pos, intersect)) {
                    particles[i].pos = old + (particles[i].pos-old)*intersect;
                    // Set velocity to 0 to make energy conserved.
                    particles[i].vel = glm::vec3(0.0f, 0.0f, 0.0f);
                }
            }

            // Adjust for ball collision.
            if (ballCol) {
                glm::vec3 diff = particles[i].pos - center;
                float diffLength = glm::length(diff);
                if (diffLength < radius) {
                    
                    //Naive way, move particle to the surface in the direction of the normal.
                    particles[i].move(glm::normalize(diff)*(radius-diffLength));
                    
                    //Advanced way (still testing, not working if ball moves), calculate the intersection and move particle to the intersection.
                    //ballIntersect(old, particles[i].pos, intersect);                    
                    //particles[i].pos = old + (particles[i].pos-old)*intersect;
                    
                    // Set velocity to 0 to make energy conserved.
                    particles[i].vel = glm::vec3(0.0f, 0.0f, 0.0f);
                }
            }

            // Adjust for cube collision.
            if (cubeCol) {
                bool insideX = particles[i].pos.x < cubeMax.x and particles[i].pos.x > cubeMin.x;
                bool insideY = particles[i].pos.y < cubeMax.y and particles[i].pos.y > cubeMin.y;
                bool insideZ = particles[i].pos.z < cubeMax.z and particles[i].pos.z > cubeMin.z;

                if (insideX and insideY and insideZ) {
                    if (hitSurface(old, particles[i].pos, intersect, v1, v2, v5, v6)) { //Left.
                        particles[i].pos = old + (particles[i].pos-old)*intersect;
                    } else if (hitSurface(old, particles[i].pos, intersect, v2, v3, v8, v5)) { //Front.
                        particles[i].pos = old + (particles[i].pos-old)*intersect;
                    } else if (hitSurface(old, particles[i].pos, intersect, v3, v4, v7, v8)) { //Right.
                        particles[i].pos = old + (particles[i].pos-old)*intersect;
                    } else if (hitSurface(old, particles[i].pos, intersect, v1, v6, v7, v4)) { //Back.
                        particles[i].pos = old + (particles[i].pos-old)*intersect;
                    } else if (hitSurface(old, particles[i].pos, intersect, v5, v8, v7, v6)) { //Top.
                        particles[i].pos = old + (particles[i].pos-old)*intersect;
                    } else if (hitSurface(old, particles[i].pos, intersect, v1, v4, v3, v2)) { //Bottom.
                        particles[i].pos = old + (particles[i].pos-old)*intersect;
                    } else {
                        cout << "This line should never be printed!!!" << endl;
                    }
                    // Set velocity to 0 to make energy conserved.
                    particles[i].vel = glm::vec3(0.0f, 0.0f, 0.0f);
                }
            }
        }
        // Reset in case of scene changes.
        ballCol = false;
        cubeCol = false;
        hasFloor = false;
    }

    // For reset to initial state.
    void reset() {
        particles = particlesBACKUP;
        sps = spsBACKUP;
    }
};

std::vector<Particle> creatParticles(int widthNum, int heightNum, float width, float height) {
    std::vector<Particle> result;
    result.resize(widthNum*heightNum);
    for(int x=0; x<widthNum; x++) {
        for(int y=0; y<heightNum; y++) {
            glm::vec3 pos = glm::vec3(width * (x/(float)widthNum),
                            0.0f,
                            height * (y/(float)heightNum));
            result[y*widthNum+x]= Particle(pos);
        }
    }
    return result;
};
