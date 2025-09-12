#include "Calculator.h"
#include "Stack.h"

#include <cstdio>
#include <ctype.h>
#include <tt_lvgl_toolbar.h>

constexpr const char* TAG = "Calculator";

static int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

void Calculator::button_event_cb(lv_event_t* e) {
    Calculator* self = static_cast<Calculator*>(lv_event_get_user_data(e));
    lv_obj_t* buttonmatrix = lv_event_get_current_target_obj(e);
    lv_event_code_t event_code = lv_event_get_code(e);
    uint32_t button_id = lv_buttonmatrix_get_selected_button(buttonmatrix);
    const char* button_text = lv_buttonmatrix_get_button_text(buttonmatrix, button_id);
    if (event_code == LV_EVENT_VALUE_CHANGED) {
        self->handleInput(button_text);
    }
}

void Calculator::handleInput(const char* txt) {
    if (strcmp(txt, "C") == 0) {
        resetCalculator();
        return;
    }

    if (strcmp(txt, "=") == 0) {
        evaluateExpression();
        return;
    }

    if (strlen(formulaBuffer) + strlen(txt) < sizeof(formulaBuffer) - 1) {
        if (newInput) {
            memset(formulaBuffer, 0, sizeof(formulaBuffer));
            newInput = false;
        }
        strcat(formulaBuffer, txt);
        lv_label_set_text(displayLabel, formulaBuffer);
    }
}

Dequeue<Str> Calculator::infixToRPN(const Str& infix) {
    Stack<char> opStack;
    Dequeue<Str> output;
    Str token;
    size_t i = 0;

    while (i < infix.length()) {
        char ch = infix[i];

        if (isdigit(ch)) {
            token.clear();
            while (i < infix.length() && (isdigit(infix[i]) || infix[i] == '.')) { token.append(infix[i++]); }
            output.pushBack(token);
            continue;
        }

        if (ch == '(') { opStack.push(ch); } else if (ch == ')') {
            while (!opStack.empty() && opStack.top() != '(') {
                output.pushBack(Str(1, opStack.top()));
                opStack.pop();
            }
            opStack.pop();
        } else if (strchr("+-*/", ch)) {
            while (!opStack.empty() && precedence(opStack.top()) >= precedence(ch)) {
                output.pushBack(Str(1, opStack.top()));
                opStack.pop();
            }
            opStack.push(ch);
        }

        i++;
    }

    while (!opStack.empty()) {
        output.pushBack(Str(1, opStack.top()));
        opStack.pop();
    }

    return output;
}

double Calculator::evaluateRPN(Dequeue<Str> rpnQueue) {
    Stack<double> values;

    while (!rpnQueue.empty()) {
        Str token = rpnQueue.front();
        rpnQueue.popFront();

        if (isdigit(token[0])) {
            double d;
            sscanf(token.c_str(), "%lf", &d);
            values.push(d);
        } else if (strchr("+-*/", token[0])) {
            if (values.size() < 2) return 0;

            double b = values.top();
            values.pop();
            double a = values.top();
            values.pop();

            if (token[0] == '+') values.push(a + b);
            else if (token[0] == '-') values.push(a - b);
            else if (token[0] == '*') values.push(a * b);
            else if (token[0] == '/' && b != 0) values.push(a / b);
        }
    }

    return values.empty() ? 0 : values.top();
}
void Calculator::evaluateExpression() {
    double result = computeFormula();

    size_t formulaLen = strlen(formulaBuffer);
    size_t maxAvailable = sizeof(formulaBuffer) - formulaLen - 1;

    if (maxAvailable > 10) {
        char resultBuffer[32];
        snprintf(resultBuffer, sizeof(resultBuffer), " = %.8g", result);
        strncat(formulaBuffer, resultBuffer, maxAvailable);
    } else { snprintf(formulaBuffer, sizeof(formulaBuffer), "%.8g", result); }

    lv_label_set_text(displayLabel, "0");
    lv_label_set_text(resultLabel, formulaBuffer);
    newInput = true;
}

double Calculator::computeFormula() {
    return evaluateRPN(infixToRPN(Str(formulaBuffer)));
}

void Calculator::resetCalculator() {
    memset(formulaBuffer, 0, sizeof(formulaBuffer));
    lv_label_set_text(displayLabel, "0");
    lv_label_set_text(resultLabel, "");
    newInput = true;
}

void Calculator::onShow(AppHandle appHandle, lv_obj_t* parent) {
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    lv_obj_t* toolbar = tt_lvgl_toolbar_create_for_app(parent, appHandle);
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

    lv_obj_t* buttonmatrix = lv_buttonmatrix_create(parent);
    lv_buttonmatrix_set_map(buttonmatrix, btn_map);

    lv_obj_set_style_pad_all(buttonmatrix, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_row(buttonmatrix, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_column(buttonmatrix, 5, LV_PART_MAIN);
    lv_obj_set_style_border_width(buttonmatrix, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(buttonmatrix, lv_palette_main(LV_PALETTE_BLUE), LV_PART_ITEMS);

    if (lv_display_get_horizontal_resolution(nullptr) <= 240 || lv_display_get_vertical_resolution(nullptr) <= 240) {
        //small screens
        lv_obj_set_size(buttonmatrix, lv_pct(100), lv_pct(60));
    } else {
        //large screens
        lv_obj_set_size(buttonmatrix, lv_pct(100), lv_pct(80));
    }
    lv_obj_align(buttonmatrix, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_add_event_cb(buttonmatrix, button_event_cb, LV_EVENT_VALUE_CHANGED, this);
}