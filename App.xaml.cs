using MalangDiary.ViewModels;
using MalangDiary.Views;
using Microsoft.Extensions.DependencyInjection;
using System.Configuration;
using System.Data;
using System.Windows;


namespace MalangDiary {
    public partial class App : Application {
        public static IServiceProvider ? Services { get; private set; }

        public App() {
            var serviceCollection = new ServiceCollection();
            ConfigureServices(serviceCollection);
            Services = serviceCollection.BuildServiceProvider();

            var socket = Services.GetService<Services.SocketManager>();
            _ = socket?.EnsureConnectedAsync();
        }

        private static void ConfigureServices(IServiceCollection services) {
            // Models
            services.AddSingleton<Models.UserSession>();
            services.AddSingleton<Models.UserModel>();
            services.AddSingleton<Models.HomeModel>();
            services.AddSingleton<Models.DiaryModel>();
            services.AddSingleton<Models.RgsModel>();
            services.AddSingleton<Models.ChkModel>();

            // Services
            services.AddSingleton<Services.SocketManager>();

            // ViewModels
            services.AddSingleton<MainViewModel>();
            services.AddTransient<StartViewModel>();
            services.AddTransient<LogInViewModel>();
            services.AddTransient<SignUpViewModel>();
            services.AddTransient<HomeViewModel>();
            services.AddTransient<RgsChdViewModel>();
            services.AddTransient<RgsChdVoiceView>();
            services.AddTransient<RcdDiaryViewModel>();
            services.AddTransient<ChkCldViewModel>();
            services.AddTransient<ChkDiaryViewModel>();
            services.AddTransient<ChkGalleryViewModel>();
        }
    }
}