#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform vec3 lightPos; // Позиция источника света
uniform vec3 viewPos;  // Позиция камеры
uniform vec3 lightColor; // Цвет источника света

uniform vec3 spotlightDirection; 

// Функции для трех видов освещения
vec4 pointLight() {
    // Интенсивность света в зависимости от расстояния
    vec3 lightVec = lightPos - FragPos;
    float dist = length(lightVec);
    float a = 3.0;
    float b = 0.7;
    float inten = 5.0f / (a * dist * dist + b * dist + 1.0f);

    // Ambient
    float ambient = 0.20f;

    // Diffuse
    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(lightVec);
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    // Specular
    float specularLight = 0.50f;
    vec3 viewDirection = normalize(viewPos - FragPos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
    float specular = specAmount * specularLight;

    // Итоговый цвет
    return (texture(texture1, TexCoord) * (diffuse * inten + ambient) + texture(texture2, TexCoord).r * specular * inten) * vec4(lightColor, 1.0);
}

vec4 direcLight() {
    // Ambient
    float ambient = 0.20f;

    // Diffuse
    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(vec3(1.0f, 1.0f, 0.0f)); // Направленный свет
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    // Specular
    float specularLight = 0.50f;
    vec3 viewDirection = normalize(viewPos - FragPos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
    float specular = specAmount * specularLight;

    // Итоговый цвет
    return (texture(texture1, TexCoord) * (diffuse + ambient) + texture(texture2, TexCoord).r * specular) * vec4(lightColor, 1.0);
}

vec4 spotLight() {
    // Параметры прожектора (можно вынести в uniform)
    float outerCone = 0.90f;  // Косинус внешнего угла (~25°)
    float innerCone = 0.95f;  // Косинус внутреннего угла (~18°)
   
    
    // Ambient
    float ambient = 0.20f;
    
    // Расчет диффузного света
    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos);
    float diffuse = max(dot(normal, lightDirection), 0.0f);
    
    // Расчет бликов
    float specularLight = 0.50f;
    vec3 viewDirection = normalize(viewPos - FragPos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
    float specular = specAmount * specularLight;
    
    // Интенсивность прожектора (попадание в конус)
    float angle = dot(spotlightDirection, -lightDirection);
    float inten = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);
    
    // Итоговый цвет
    return (texture(texture1, TexCoord) * (diffuse * inten + ambient) + 
           texture(texture2, TexCoord).r * specular * inten) * vec4(lightColor, 1.0);

}

void main() {
    // Выберите тип освещения (pointLight, direcLight, spotLight)
     FragColor = pointLight(); // По умолчанию точечный свет
     //FragColor = direcLight(); // Направленный свет
     //FragColor = spotLight(); // Прожектор
}