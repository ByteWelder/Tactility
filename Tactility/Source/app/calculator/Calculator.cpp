#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/Assets.h>
#include <cstdlib>
#include <cstring>
#include <lvgl.h>
#include <queue>
#include <stack>

namespace tt::app::calculator {

constexpr const char* TAG = "Calculator";

class CalculatorApp : public App {

    lv_obj_t* displayLabel;
    lv_obj_t* resultLabel;
    char formulaBuffer[128] = {0}; // Stores the full input expression
    bool newInput = true;

    static void button_event_cb(lv_event_t* e) {
        CalculatorApp* self = static_cast<CalculatorApp*>(lv_event_get_user_data(e));
        lv_obj_t* btnm = lv_event_get_current_target_obj(e);
        lv_event_code_t code = lv_event_get_code(e);

        uint32_t btn_id = lv_buttonmatrix_get_selected_button(btnm);
        const char* txt = lv_buttonmatrix_get_button_text(btnm, btn_id);

        if (code == LV_EVENT_VALUE_CHANGED) {
            self->handleInput(txt);
        }
    }

    void handleInput(const char* txt) {
        if (std::strcmp(txt, "C") == 0) {
            resetCalculator();
            return;
        }

        if (std::strcmp(txt, "=") == 0) {
            evaluateExpression();
            return;
        }

        if (std::strlen(formulaBuffer) + std::strlen(txt) < sizeof(formulaBuffer) - 1) {
            if (newInput) {
                std::memset(formulaBuffer, 0, sizeof(formulaBuffer));
                newInput = false;
            }
            std::strcat(formulaBuffer, txt);
            lv_label_set_text(displayLabel, formulaBuffer);
        }
    }

    void evaluateExpression() {
        double result = computeFormula();

        size_t formulaLen = std::strlen(formulaBuffer);
        size_t maxAvailable = sizeof(formulaBuffer) - formulaLen - 1;

        if (maxAvailable > 10) {
            char resultBuffer[32];
            snprintf(resultBuffer, sizeof(resultBuffer), " = %.8g", result);
            strncat(formulaBuffer, resultBuffer, maxAvailable);
        } else {
            snprintf(formulaBuffer, sizeof(formulaBuffer), "%.8g", result);
        }

        lv_label_set_text(displayLabel, "0");
        lv_label_set_text(resultLabel, formulaBuffer);
        newInput = true;
    }

    double computeFormula() {
        return evaluateRPN(infixToRPN(std::string(formulaBuffer)));
    }

    int precedence(char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        return 0;
    }

    std::queue<std::string> infixToRPN(const std::string& infix) {
        std::stack<char> opStack;
        std::queue<std::string> output;
        std::string token;
        size_t i = 0;

        while (i < infix.size()) {
            char ch = infix[i];

            if (isdigit(ch)) {
                token.clear();
                while (i < infix.size() && (isdigit(infix[i]) || infix[i] == '.')) {
                    token += infix[i++];
                }
                output.push(token);
                continue;
            }

            if (ch == '(') {
                opStack.push(ch);
            } else if (ch == ')') {
                while (!opStack.empty() && opStack.top() != '(') {
                    output.push(std::string(1, opStack.top()));
                    opStack.pop();
                }
                opStack.pop();
            } else if (strchr("+-*/", ch)) {
                while (!opStack.empty() && precedence(opStack.top()) >= precedence(ch)) {
                    output.push(std::string(1, opStack.top()));
                    opStack.pop();
                }
                opStack.push(ch);
            }

            i++;
        }

        while (!opStack.empty()) {
            output.push(std::string(1, opStack.top()));
            opStack.pop();
        }

        return output;
    }

    double evaluateRPN(std::queue<std::string> rpnQueue) {
        std::stack<double> values;

        while (!rpnQueue.empty()) {
            std::string token = rpnQueue.front();
            rpnQueue.pop();

            if (isdigit(token[0])) {
                values.push(std::stod(token));
            } else if (strchr("+-*/", token[0])) {
                if (values.size() < 2) return 0;

                double b = values.top();
                values.pop();
                double a = values.top();
                values.pop();

                if (token[0] == '+') values.push(a + b);
                else if (token[0] == '-')
                    values.push(a - b);
                else if (token[0] == '*')
                    values.push(a * b);
                else if (token[0] == '/' && b != 0)
                    values.push(a / b);
            }
        }

        return values.empty() ? 0 : values.top();
    }

    void resetCalculator() {
        std::memset(formulaBuffer, 0, sizeof(formulaBuffer));
        lv_label_set_text(displayLabel, "0");
        lv_label_set_text(resultLabel, "");
        newInput = true;
    }

    void onShow(AppContext& context, lv_obj_t* parent) override {
        lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

        lv_obj_t* toolbar = lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t* wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_height(wrapper, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(wrapper, 0);
        lv_obj_set_style_pad_top(wrapper, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(wrapper, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_left(wrapper, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_right(wrapper, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_column(wrapper, 40, LV_PART_MAIN);
        lv_obj_set_style_border_width(wrapper, 0, 0);
        lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);

        displayLabel = lv_label_create(wrapper);
        lv_label_set_text(displayLabel, "0");
        lv_obj_set_width(displayLabel, LV_SIZE_CONTENT);
        lv_obj_set_align(displayLabel, LV_ALIGN_LEFT_MID);

        resultLabel = lv_label_create(wrapper);
        lv_label_set_text(resultLabel, "");
        lv_obj_set_width(resultLabel, LV_SIZE_CONTENT);
        lv_obj_set_align(resultLabel, LV_ALIGN_RIGHT_MID);

        static const char* btn_map[] = {
            "(", ")", "C", "/", "\n",
            "7", "8", "9", "*", "\n",
            "4", "5", "6", "-", "\n",
            "1", "2", "3", "+", "\n",
            "0", "=", "", "", ""
        };

        lv_obj_t* btnm = lv_btnmatrix_create(parent);
        lv_btnmatrix_set_map(btnm, btn_map);

        lv_obj_set_style_pad_all(btnm, 5, LV_PART_MAIN);
        lv_obj_set_style_pad_row(btnm, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_column(btnm, 5, LV_PART_MAIN);
        lv_obj_set_style_border_width(btnm, 2, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btnm, lv_palette_main(LV_PALETTE_BLUE), LV_PART_ITEMS);

        if (lv_display_get_horizontal_resolution(nullptr) <= 240 || lv_display_get_vertical_resolution(nullptr) <= 240) { //small screens
            lv_obj_set_size(btnm, lv_pct(100), lv_pct(60));
        } else { //large screens
            lv_obj_set_size(btnm, lv_pct(100), lv_pct(80));
        }
        lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, -5);

        lv_obj_add_event_cb(btnm, button_event_cb, LV_EVENT_VALUE_CHANGED, this);
    }
};

extern const AppManifest manifest = {
    .appId = "Calculator",
    .appName = "Calculator",
    .appIcon = TT_ASSETS_APP_ICON_CALCULATOR,
    .createApp = create<CalculatorApp>
};

} // namespace tt::app::calculator
