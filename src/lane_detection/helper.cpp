#include <vector>
#include <unistd.h>
#include <opencv2/opencv.hpp>

const int IMAGE_HEIGHT = 400;
const int IMAGE_WIDTH = 400;
const int COLOR_DIFFERENCE = 5;

class InvalidPoint : public std::exception {};
//class InvalidCircle : public std::exception {};
class UnfittingLaneWidth : public std::exception {};

class Helper {
    public:
        cv::Mat* matrix;
        cv::Mat* drawMatrix;

        double calc_dist(cv::Point p1, cv::Point p2) {
            return calc_dist(std::pair(p1.x, p1.y), std::pair(p2.x, p2.y));
        }

        double calc_dist(std::pair<double, double> p1, std::pair<double, double> p2) {
            return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
        }

        int calculate_radius(cv::Mat* matrix, cv::Mat* drawMatrix) {
            this->matrix = matrix;
            this->drawMatrix = drawMatrix;

            cv::cvtColor(*this->drawMatrix, *this->drawMatrix, cv::COLOR_GRAY2RGB);
            
            const int INITIAL_RADIUS = 50;
            const int FINAL_RADIUS = 250;

            cv::Point* previous_center = nullptr;
            int previous_center_radian = -1;

            std::vector<cv::Point> center_point_list;

            for(int radius = INITIAL_RADIUS; radius < FINAL_RADIUS; radius += 25) {
                std::vector<cv::Point> point_list = get_pointlist_of_radius(radius);

                if(previous_center != nullptr) {
                    float dy = previous_center->y - 400;
                    float dx = previous_center->x - 200;
                    previous_center_radian = int(std::atan2(dx, dy) * 100);
                }
                
                try {
                    cv::Point point = get_street_middle_from_points(point_list, previous_center_radian, radius);

                    center_point_list.push_back(point);
                    previous_center = &point;

                    cv::circle(*drawMatrix, point, 2, cv::Scalar(0, 255, 255, 1), 2);
                } catch(const UnfittingLaneWidth& e) {} // Just an unfitting lane width so no need to worry abt an actual error
            }

            /*cv::Point final_center = cv::Point(200,400);
            int final_radius = 0;

            try {
                auto val = fit_circle_with_fixed_point_ransac(center_point_list, cv::Point(200, 400));
                std::tie(final_center, final_radius) = val;
            } catch(const InvalidCircle& e) {} // Just an unfitting lane width so no need to worry abt an actual error
            
            cv::circle(*drawMatrix, final_center, final_radius, cv::Scalar(255,0,0,1), 1);*/

            for(int radius = INITIAL_RADIUS; radius < FINAL_RADIUS; radius += 25) {
                cv::circle(*drawMatrix, cv::Point(200, 400), radius, cv::Scalar(255,255,255,1), 1);
            }

            #ifdef DEBUG
                cv::imshow("Lane Detection", *drawMatrix);
                char key = cv::waitKey(30);
                if (key == 'q')
                {
                    cv::destroyAllWindows();
                    return 0;
                }
            #endif

            cv::Point final_center;
            int final_radius;

            std::tie(final_center, final_radius) = loop_through_circles(center_point_list);

            cv::circle(*this->drawMatrix, final_center, abs(final_radius), cv::Scalar(200, 110, 50, 255), 5);

            return final_radius * (final_center.x < 200 ? -1 : 1);
        }

        double radius_to_angle(float radius) {
            if(radius > 0) {
                return 3000000.0 * std::pow(radius, -3) + 10;
            } else if(radius < 0) {
                return 2700000.0 * std::pow(radius, -3) - 10;
            } else {
                return 0;
            }
        }

    private:
        void get_two_random_element_indexes (const int size, int &first, int &second) {
            // pick a random element
            first = ((float)rand()) / RAND_MAX * (size - 1);
            // pick a random element from what's left (there is one fewer to choose from)...
            second = ((float)rand()) / RAND_MAX * (size - 1);
            // ...and adjust second choice to take into account the first choice
            if (second == first && second != 0)
            {
                --second;
            } else if (second == first && second == 0)
            {
                ++second;
            }
        }

        cv::Point check_for_valid_point(int direction, int radius, float looking_pi) {
            std::pair<int, int> pair[2];

            for (double pi = 0; pi < 1; pi += 0.001) {
                double offset = looking_pi + pi * direction;

                int x = 200 + round(cos(offset) * radius);
                int y = 400 - round(sin(offset) * radius);

                int x2 = 200 + round(cos(offset+0.01 * direction) * radius);
                int y2 = 400 - round(sin(offset+0.01 * direction) * radius);

                int x3 = 200 + round(cos(offset+0.03 * direction) * radius);
                int y3 = 400 - round(sin(offset+0.03 * direction) * radius);

                if (x >= IMAGE_WIDTH || x < 0 || y >= IMAGE_HEIGHT || y < 0) {
                    continue;
                }

                if (x2 >= IMAGE_WIDTH || x2 < 0 || y2 >= IMAGE_HEIGHT || y2 < 0) {
                    continue;
                }

                if (x3 >= IMAGE_WIDTH || x3 < 0 || y3 >= IMAGE_HEIGHT || y3 < 0) {
                    continue;
                }

                int color = matrix->at<uint8_t>(y, x);
                int color2 = matrix->at<uint8_t>(y2, x2);
                int color3 = matrix->at<uint8_t>(y3, x3);

                if (color > 235) {
                    continue;
                }

                if ((color2 - color) * direction > COLOR_DIFFERENCE && (color3 - color) * direction > COLOR_DIFFERENCE) {
                    pair[0] = std::pair(x,y);
                }

                if((color - color2) * direction > COLOR_DIFFERENCE && pair[0].first != 0 && pair[0].second != 0 && (color - color3) * direction > COLOR_DIFFERENCE) {
                    pair[1] = std::pair(x,y);

                    double dist = calc_dist(pair[0], pair[1]);

                    if(dist <= 10.0f) {
                        return cv::Point((pair[0].first + pair[1].first) / 2, (pair[0].second + pair[1].second) / 2);
                    }
                }
            }

            throw InvalidPoint();
        }

        cv::Point get_street_middle_from_points(std::vector<cv::Point> point_list, int previous_center, int radius) {
            if (previous_center == -1) {
                previous_center = 3.14;
            }
                
            int x = 200 + round(std::sin(previous_center / 100) * radius);
            int y = 400 + round(std::cos(previous_center / 100) * radius);

            cv::circle(*this->drawMatrix, cv::Point(int(x), int(y)), 10, cv::Scalar(255,255,255,0), 2); // white


            for(cv::Point point : point_list) {
                cv::circle(*this->drawMatrix, point, 20, cv::Scalar(0,255,0,0), 1); //green
            }

            std::vector<cv::Point> right_pointlist, left_pointlist;

            for (const auto& point : point_list) {
                if (point.x - x > 0) {
                    right_pointlist.push_back(point);
                } else {
                    left_pointlist.push_back(point);
                }
            }

            for(cv::Point left : left_pointlist) {
                cv::circle(*this->drawMatrix, left, 20, cv::Scalar(0,0,255,0), 1); //red
            }

            std::sort(right_pointlist.begin(), right_pointlist.end(), [&](const cv::Point& a, const cv::Point& b) {
                return a.x - x < b.x - x;
            });

            std::sort(left_pointlist.begin(), left_pointlist.end(), [&](const cv::Point& a, const cv::Point& b) {
                return abs(a.x - x) < abs(b.x - x);
            });

            if(right_pointlist.empty() && left_pointlist.empty()) {
                throw UnfittingLaneWidth();
            }

            if(right_pointlist.empty()) {
                return cv::Point(left_pointlist[0].x - 25, left_pointlist[0].y);
            }

            if(left_pointlist.empty()) {
                return cv::Point(right_pointlist[0].x - 25, right_pointlist[0].y);
            }

            double dist = calc_dist(right_pointlist[0], left_pointlist[0]);

            if(dist < 70) {
                return cv::Point((right_pointlist[0].x + left_pointlist[0].x) / 2, (right_pointlist[0].y + left_pointlist[0].y) / 2);
            } else if(dist >= 70 && dist <= 130) {
                return cv::Point((right_pointlist[0].x + left_pointlist[0].x) / 4, (right_pointlist[0].y + left_pointlist[0].y) / 4);
            } else {
                throw UnfittingLaneWidth();
            }
        }

        std::vector<cv::Point> get_pointlist_of_radius(int radius) {
            std::vector<cv::Point> point_list;

            for(float pi = 0; pi < 3.14; pi += 0.001f) {
                try {
                    cv::Point point = check_for_valid_point(1, radius, pi);

                    if(std::find(point_list.begin(), point_list.end(), point) == point_list.end()) {
                        point_list.push_back(point);
                    }
                } catch(const InvalidPoint& e) {} // Just an invalid point so no need to worry abt an actual error
            }

            return point_list;
        }

        /*std::pair<cv::Point, int> calculate_circle_center_radius(cv::Point p1, cv::Point p2, cv::Point fixed_point) {
            float ma;
            float mb;

            if(p2.x != p1.x && p2.y != p1.y) {
                ma = ((float)(p2.y - p1.y)) / (p2.x - p1.x);
            } else {
                ma = std::numeric_limits<float>::infinity();
            }

            if(fixed_point.x != p2.x && fixed_point.y != p2.y) {
                mb = ((float)(fixed_point.y - p2.y)) / (fixed_point.x - p2.x);
            } else {
                mb = std::numeric_limits<float>::infinity();
            }

            if(ma == mb) {
                throw InvalidCircle();
            }

            int cx = (ma * mb * (p1.y - fixed_point.y) + mb * (p1.x + p2.x) - ma * (p2.x + fixed_point.x)) / (2 * (mb - ma));
            int cy;

            if(ma != std::numeric_limits<float>::infinity()) {
                cy = -1 * (cx - (p1.x + p2.x) / 2) / ma + (p1.y + p2.y) / 2;
            } else {
                cy = (cx - (p2.x + fixed_point.x) / 2) / mb + (p2.y + fixed_point.y) / 2;
            }

            int radius = std::sqrt(std::pow(cx - p1.x, 2) + std::pow(cy - p1.y, 2));

            return std::pair(cv::Point(cx, cy), radius);
        }*/

        /*std::pair<cv::Point, int> fit_circle_with_fixed_point_ransac(std::vector<cv::Point> points, cv::Point fixed_point) {
            const int MAX_ITERATIONS = 1000;
            const float DISTANCE_THRESHOLD = 1;
            const float MIN_INLIERS_RATIO = 0.5;

            std::pair<cv::Point, int>* best_circle = nullptr;
            std::vector<cv::Point> best_inliners;
            int num_points = points.size();

            if(num_points == 0) {
                throw InvalidCircle();
            }

            for (int _ = 0; _ < MAX_ITERATIONS; _++) {
                int first_index;
                int second_index;

                get_two_random_element_indexes(points.size(), first_index, second_index);

                cv::Point sample[2] = {
                    points[first_index],
                    points[second_index]
                };

                std::pair<cv::Point, int> circle_center_radius;

                try {
                    circle_center_radius = calculate_circle_center_radius(sample[0], sample[1], fixed_point);
                } catch(const InvalidCircle& e) { // Just an invalid circle so no need to worry abt an actual error
                    continue;
                }
                
                cv::Point center = circle_center_radius.first;
                int radius = circle_center_radius.second;

                std::vector<cv::Point> inliers;

                for(cv::Point p : points) {
                    if(std::abs(std::sqrt(std::pow(p.x - center.x, 2) + std::pow(p.y - center.y, 2)) - radius) < DISTANCE_THRESHOLD) {
                        inliers.push_back(p);
                    }
                }

                if(inliers.size() > MIN_INLIERS_RATIO * num_points && inliers.size() > best_inliners.size()) { // TODO: Zu wenig inliers
                    best_circle = &circle_center_radius;
                    best_inliners = inliers;
                }
            }

            if(best_circle == nullptr) {
                throw InvalidCircle();
            }

            int h, k ,r;
            
            std::tie(h, k ,r) = fit_circle_with_fixed_point(points, fixed_point);

            return std::pair(cv::Point(h, k), r);
        }

        std::tuple<int, int, int> fit_circle_with_fixed_point(std::vector<cv::Point> points, cv::Point fixed_point) {
            int x_fixed, y_fixed; 
            x_fixed = fixed_point.x;
            y_fixed = fixed_point.y;

            std::vector<int> x_shifted, y_shifted;

            for(cv::Point p : points) {
                x_shifted.push_back(p.x - x_fixed);
                y_shifted.push_back(p.y - y_fixed);
            }

            cv::Mat A(points.size(), 2, CV_32F);
            cv::Mat b(points.size(), 1, CV_32F);

            for (size_t i = 0; i < points.size(); ++i) {
                A.at<float>(i, 0) = 2 * x_shifted[i];
                A.at<float>(i, 1) = 2 * y_shifted[i];
                b.at<float>(i, 0) = std::pow(x_shifted[i], 2) + std::pow(y_shifted[i], 2);
            }

            cv::Mat h_k_shifted;
            cv::solve(A, b, h_k_shifted, cv::DECOMP_NORMAL);

            float h_shifted = h_k_shifted.at<float>(0, 0);
            float k_shifted = h_k_shifted.at<float>(1, 0);

            // Step 4: Convert the coordinates back to the original coordinate system
            float h = h_shifted + x_fixed;
            float k = k_shifted + y_fixed;

            float r = std::sqrt(std::pow(h - x_fixed, 2) + std::pow(k - y_fixed, 2));

            return std::make_tuple(static_cast<int>(h), static_cast<int>(k), static_cast<int>(r));
        }*/

        std::tuple<cv::Point, int> loop_through_circles(std::vector<cv::Point> points) {
            int best_radius = 100000000;
            cv::Point best_center = cv::Point(0,0);
            int best_dist = 100000000;

            for(float ra = -5; ra < 5; ra+= 0.1) {
                if (std::abs(ra) < 11) {
                    continue;
                }

                int r = int(std::pow(ra, 5));
                cv::Point center = cv::Point(200+r, 400);

                double dist = 0;

                for (cv::Point point : points) {
                    dist += abs(calc_dist(center, point) - abs(r));
                }

                if (dist < best_dist) {
                    best_dist = dist;
                    best_radius = r;
                    best_center = center;
                }
            }

            return std::make_tuple(best_center, best_radius);
        }
};